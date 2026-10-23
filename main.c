#include <pthread.h>
#include <microhttpd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <execinfo.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <limits.h>
#include "main.h"
#include "settings.h"
#include "job_list.h"
#include "script_list.h"
#include "http.h"
#include "utf8.h"

#define LINE()  do{ *((int *)_line) = (int) __LINE__;}while(0);

static int gently_kill(pid_t pid, int *exit_status){
    pid_t r = waitpid(pid, exit_status, WNOHANG);
    if( r == -1) goto ERROR;
    if( r == pid) return 0;
    if(kill(pid, SIGTERM))  goto ERROR;
    for(int i=0; i<30; i++){
        r = waitpid(pid, exit_status, WNOHANG);
        if( r == -1)    goto ERROR;
        if( r == pid )  return 0;
        usleep(100000);// 100ms
    }
    if(kill(pid, SIGKILL))  goto ERROR;
    r = waitpid(pid, exit_status, 0);
    if( r == -1) goto ERROR;
    return 0;
ERROR:
    perror("gently_kill");
    return -1;
}

/* 前回の更新の改行以降は残して追記 */
static ssize_t readbuf(int fd, char *buf, size_t bufsize){
    char c;
    ssize_t n = read(fd, &c, 1);
    if( n <= 0 ) return n;

    size_t keep = 0;
    char *nl = strrchr(buf, '\n');
    if (nl) {
        keep = strlen(nl + 1);
        memmove(buf, nl + 1, keep + 1);
    }
    *(buf + keep) = c;
    keep++;
    
    n = read(fd, buf + keep, bufsize - keep - 1);
    if (n > 0){
        int m = utf8_missing((unsigned char*)buf, keep + n);
        if( m < 0 )
            return n;
        for(int i = 0; i < m; i++){
            if(bufsize <= keep + n +1 )
                break;
            ssize_t r = read(fd, buf + keep + n, 1);
            if( r <= 0)
                break;
            n++;
        }
        buf[keep + n] = '\0';
    }
    return n;
}

static void *worker(void *_line){
    DBG("worker\n");
    char token[12];
    for(;;){
        DBG("> job_next.\n");LINE();
        Job_t *job = job_next();
        atomic_store(&(job->status), processing);
        job_clear_buf(job);
        job->ec = 0;
        snprintf(token, sizeof token, "%d", job->token);
        setenv("TOKEN", token, 1); 

        DBG("> new job: ");DBG_JOB(job);LINE();
        char pathname[1024];
        snprintf(pathname, sizeof pathname, "%s/%s", conf.script_dir, job->scriptfile);
        DBG("> pathname = %s\n",pathname);

        LINE();
        int outfd[2], errfd[2];
        if (pipe(outfd) == -1) goto ERROR;
        if (pipe(errfd) == -1) goto ERROR;
        LINE();
        pid_t pid = fork();
        if (pid == -1) goto ERROR;
        if (pid == 0) {
            close(outfd[0]);
            close(errfd[0]);
            dup2(outfd[1], STDOUT_FILENO);
            dup2(errfd[1], STDERR_FILENO);
            close(outfd[1]);
            close(errfd[1]);
            int err = execl(pathname, job->scriptfile, job->post, NULL);
            if(err){
                eprintf("❌ ERROR! at execl.\n");
                perror("error");
            }
            _exit(1);
        }

        LINE();
        job->pid = pid;
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
                ssize_t n = readbuf(outfd[0], job->out, JOB_LINE_LENGTH);
                if ( n == 0 ){
                    close(outfd[0]);
                    outfd[0] = -1;
                } else if( n < 0){
                    goto ERROR;
                }
                DBG("[out]\n%s", job->out);LINE();
            }
            LINE();
            if (errfd[0] != -1 && FD_ISSET(errfd[0], &rfds)) {
                ssize_t n = readbuf(errfd[0], job->err, JOB_LINE_LENGTH);
                if ( n == 0 ){
                    close(errfd[0]);
                    errfd[0] = -1;
                } else if( n < 0 ){
                    goto ERROR;
                }
                DBG("[err]\n%s", job->err);LINE();
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
        LINE();
        if(job->ec){
            atomic_store(&(job->status), error);
        }
        DBG("job finished: ");DBG_JOB(job);LINE();
        continue;
ERROR:
        LINE();
        eprintf("❌ ERROR! at job %s %s.\n", job->scriptfile, job->post);
        perror("error");
        atomic_store(&(job->status), error);
    }
}

static atomic_int   _sig;
static atomic_bool  _running = true;
static void sig_handler(int sig) {
    _sig = sig;
    _running = false;
}

int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);

    DBG("settings_load()\n");
    settings_load();
    DBG("script_load()\n");
    script_load();
    DBG("job_load()\n");
    job_load();
    
    DBG("create pthread\n");
    int nthread = conf.nthread;
    int _line[nthread];
    pthread_t thread[nthread];
    for(int i=0; i<nthread; i++){
        pthread_create(&thread[i], NULL, worker, &_line[i]);
        pthread_detach(thread[i]);
    }

    DBG("start MHD daemon\n");
    struct MHD_Daemon *local_daemon = NULL;
    struct MHD_Daemon *public_daemon = NULL;
    App_data local_data  = { .is_local = 1 };
    App_data public_data = { .is_local = 0 };
    if( 0 < conf.lport){
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(conf.lport);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        local_daemon = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD,
                0,
                &accept_policy, NULL,
                &access_handler, &local_data,
                MHD_OPTION_SOCK_ADDR, (struct sockaddr *)&addr,
                MHD_OPTION_END
                );
        if(!local_daemon){
            perror("failed to start local http server.\n");
            goto FINALIZE;
        }
        eprintf("start local http server. waiting at :%d\n", conf.lport);
    }
    if (conf.cert && conf.key) {
        public_daemon = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS,
                conf.port,
                &accept_policy, NULL,
                &access_handler, &public_data,
                MHD_OPTION_HTTPS_MEM_CERT, conf.cert,
                MHD_OPTION_HTTPS_MEM_KEY,  conf.key,
                MHD_OPTION_END
                );
        if(!public_daemon){
            perror("failed to start public https server.\n");
            goto FINALIZE;
        }
        eprintf("start public http server. waiting at :%d\n", conf.port);
    }else if(conf.force_https){
        perror("FORCE_HTTPS=yes. stop starting https server.\n");
        goto FINALIZE;
    }else{
        public_daemon = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD,
                conf.port,
                &accept_policy, NULL,
                &access_handler, &public_data,
                MHD_OPTION_END
                );
        if(!public_daemon){
            perror("failed to start public http server.\n");
            goto FINALIZE;
        }
        eprintf("start http server. waiting at :%d .\n", conf.port);
    }

    while(_running){
        pause();
    }

FINALIZE:
    eprintf("stop server.\n");
    if(local_daemon)  MHD_stop_daemon(local_daemon);
    if(public_daemon) MHD_stop_daemon(public_daemon);

    job_unload();
    script_unload();
    settings_unload();

    eprintf("=== server stoped ===\n");
    eprintf("signal:           %d\n", _sig);
    for(int i=0; i<nthread; i++){
    eprintf("%dth thread line: %d\n", i, _line[i]);
    }
    void *buf[32];
    int n = backtrace(buf, 32);
    backtrace_symbols_fd(buf, n, STDERR_FILENO);
  
    return 0;
}
