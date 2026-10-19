#include <pthread.h>
#include <microhttpd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <execinfo.h>
#include <stdlib.h>
#include "debug.h"
#include "config.h"
#include "job_list.h"
#include "script_list.h"
#include "metadata.h"
#include "http.h"

#define PTHREAD_MAX 2

#define LINE()    do{_line = __LINE__;}while(0);
static int _line = 0;

static int gently_kill(pid_t pid, int *exit_status){
    pid_t r = waitpid(pid, exit_status, WNOHANG);
    if(r == pid) return 0;
    if(kill(pid, SIGTERM))  return -1;
    for(int i=0; i<30; i++){
        pid_t r = waitpid(pid, exit_status, WNOHANG);
        if( r == -1)    return -1;
        if( r == pid )    return 0;
        usleep(100000);// 100ms
    }
    if(kill(pid, SIGKILL))  return -1;
    return 0;
}

static void *worker(void *arg){
    (void)arg;
    DBG(" in worker.\n");
    for(;;){
        DBG(" > job_next.\n");LINE();
        Job_t *job = job_next();
        atomic_store(&(job->status), processing);
        job_clear_buf(job);

        DBG(" > forke.\n");LINE();
        char pathname[1024];
        snprintf(pathname, sizeof pathname, "%s/%s", conf.script_dir, job->scriptfile);
        DBG(" pathname = %s\n",pathname);
        int outfd[2], errfd[2];
        if (pipe(outfd) == -1) goto ERROR;
        if (pipe(errfd) == -1) goto ERROR;
        LINE();
        pid_t pid = fork();
        if (pid == -1) goto ERROR;
        if (pid == 0) {
            DBG(" forked in.\n");
            close(outfd[0]);
            close(errfd[0]);
            dup2(outfd[1], STDOUT_FILENO);
            dup2(errfd[1], STDERR_FILENO);
            close(outfd[1]);
            close(errfd[1]);
            int err = execl(pathname, job->scriptfile, job->post, NULL);
            if(err){
                printf("❌ ERROR! at execl.\n");
                perror("error");
            }
            _exit(1);
        }

        DBG(" > forked.\n");LINE();
        close(outfd[1]);
        close(errfd[1]);
        while(job->status == processing){
            LINE();
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1100000; //1.1s

            LINE();
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(outfd[0], &rfds);
            FD_SET(errfd[0], &rfds);

            LINE();
            int maxfd = outfd[0] > errfd[0] ? outfd[0] : errfd[0];
            int ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
            if (ret == -1) goto ERROR;
            if (ret == 0 ) continue;//timeout

            LINE();
            if (outfd[0] != -1 && FD_ISSET(outfd[0], &rfds)) {
                ssize_t n = read(outfd[0], job->out, JOB_LINE_LENGTH - 1);
                /* DBG("read from stdout: %ld\n", n); */
                if (n > 0) {
                    job->out[n] = '\0';
                } else if ( n == 0 ){
                   close(outfd[0]);
                   outfd[0] = -1;
                } else {
                    goto ERROR;
                }
            }
            LINE();
            if (errfd[0] != -1 && FD_ISSET(errfd[0], &rfds)) {
                ssize_t n = read(errfd[0], job->err, JOB_LINE_LENGTH - 1);
                /* DBG("read from stderr: %ld\n", n); */
                if (n > 0) {
                    job->err[n] = '\0';
                } else if ( n == 0 ){
                   close(errfd[0]);
                   errfd[0] = -1;
                } else {
                    goto ERROR;
                }
            }
            LINE();
            if(outfd[0] == -1 && errfd[0] == -1)
                break;//EOF
        }
        LINE();
        int status;
        if(gently_kill(pid, &status))   goto ERROR;
        job->ec = WEXITSTATUS(status);
        LINE();
        switch(job->status){
            case processing:
                atomic_store(&(job->status), finished);
                break;
            case canceling:
                atomic_store(&(job->status), canceled);
                break;
            case deleting:
                atomic_store(&(job->status), deleted);
                break;
            default:
                goto ERROR;
        }
        DBG(" > script's exit code = %d\n", job->ec);LINE();
        if(job->ec){
            atomic_store(&(job->status), error);
        }
        continue;
ERROR:
        LINE();
        printf("❌ ERROR! at job %s %s.\n", job->scriptfile, job->post);
        perror("error");
        atomic_store(&(job->status), error);
    }
}

static int _sig;
static volatile sig_atomic_t _running = 1;
static void sig_handler(int sig) {
    dprintf(STDOUT_FILENO, "=== signal catched ===\n");
    _sig = sig;
    _running = 0;
}

int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
    
    DBG("config_load()\n");
    config_load();
    DBG("conf.cert %.26s\n", conf.cert);
    DBG("conf.key  %.26s\n", conf.key);

    DBG("script_load()\n");
    script_load();
    DBG("job_load()\n");
    job_load();
    
    DBG("create pthread\n");
    pthread_t thread[PTHREAD_MAX];
    for(int i=0; i<PTHREAD_MAX; i++){
        pthread_t th = thread[i];
        pthread_create(&th, NULL, worker, NULL);
        pthread_detach(th);
    }
    DBG("start MHD daemon\n");
    char mes[256] = {0};
    struct MHD_Daemon *d = NULL;
    if (conf.cert && conf.key) {
        d = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS,
                conf.port,
                &accept_policy, NULL,
                &access_handler, NULL,
                MHD_OPTION_HTTPS_MEM_CERT, conf.cert,
                MHD_OPTION_HTTPS_MEM_KEY,  conf.key,
                MHD_OPTION_END
                );
        snprintf(mes, sizeof mes, "start https server. waiting at :%d\n", conf.port);
    } else if (conf.force_https){
        snprintf(mes, sizeof mes, "FORCE_HTTPS=yes. stop starting https server.\n");
    } else {
        d = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD,
                conf.port,
                &accept_policy, NULL,
                &access_handler, NULL,
                MHD_OPTION_END
                );
        snprintf(mes, sizeof mes, "start http server. waiting at :%d .\n", conf.port);
    }

    if (!d) {
        if(strlen(mes)){
            fprintf(stderr, "%s", mes);
        }
        printf("failed to start server.\n");
        fflush(stdout);
        fflush(stderr);
        return 1;
    }
    printf("%s", mes);
    fflush(stdout);
    fflush(stderr);

    while(_running){
        pause();
    }

    printf("stop server.\n");
    MHD_stop_daemon(d);

    job_unload();
    script_unload();
    config_unload();

    dprintf(STDOUT_FILENO, "=== server stoped ===\n");
    dprintf(STDOUT_FILENO, "signal: %d\n", _sig);
    dprintf(STDOUT_FILENO, "line  : %d\n", _line);
    void *buf[32];
    int n = backtrace(buf, 32);
    backtrace_symbols_fd(buf, n, STDOUT_FILENO);
  
    return 0;
}
