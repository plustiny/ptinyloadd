#include <pthread.h>
#include <jansson.h>
#include <microhttpd.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <openssl/md5.h>
#include <execinfo.h>
#include <zlib.h>
#include "debug.h"
#include "config.h"
#include "job_list.h"
#include "script_list.h"
#include "metadata.h"

#define GET "GET"
#define POST "POST"

#define LINE()    do{_line = __LINE__;}while(0);
static int _line = 0;

static enum MHD_Result _debug_print_header_inner(void *cls, enum MHD_ValueKind kind,
        const char *key, const char *value){
    (void)cls;(void)kind;
    printf("[%s]:[%s]\n", key, value);
    return MHD_YES;
}

static void _debug_print_header(struct MHD_Connection *connection){
    MHD_get_connection_values(connection, MHD_HEADER_KIND,
            &_debug_print_header_inner,NULL);
}


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

//echo -n "$str" | md5sum
static char *md5str(const char *str){
    char out[2 * MD5_DIGEST_LENGTH + 1];
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((const unsigned char *)str, strlen(str), digest);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(out + i * 2, "%02x", digest[i]);
    }
    out[MD5_DIGEST_LENGTH * 2] = '\0';
    return strdup(out);
}

static int md5cmp(char *plane, char *md5){
    char *pmd5 = md5str(plane);
    int ret = strcmp(pmd5, md5);
    free(pmd5);
    return ret;
}

static int http_auth(char* name, char* passwd){
    DBG("authorizing %s %s\n", name, passwd);
    if(strcmp(name, conf.name)){
        DBG("name differ %s %s.\n", name, conf.name);
        return -1;
    }
    if(conf.passwd_md5){
        if(md5cmp(passwd, conf.passwd_md5)){
            DBG("passwd_md5 differ.\n");
            return -1;
        }
        DBG("passwd_md5 matched.\n");
        return 0;
    }else if(conf.passwd){
        if(strcmp(passwd, conf.passwd)){
            DBG("passwd differ.\n");
            return -1;
        }
        DBG("passwd matched.\n");
        return 0;
    }
    printf("❌ ERROR! name or password are not set.\n");
    return -1;
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
            tv.tv_usec = 1000000; //1s

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
        continue;
ERROR:
        LINE();
        printf("❌ ERROR! at job %s %s.\n", job->scriptfile, job->post);
        perror("error");
        atomic_store(&(job->status), error);
    }
}

static void query_restart(const char *id, Job_status status){
    DBG(" in query_restart.\n");
    job_change_status(id, status, unprocessed, processing);
}

static void query_cancel(const char *id, Job_status status){
    DBG(" in query_cancel.\n");
    job_change_status(id, status, canceling, canceled);
}

static void query_delete(const char *id, Job_status status){
    DBG(" in query_delete.\n");
    job_change_status(id, status, deleting, deleted);
}

static char *query_select(uint32_t max, Job_status status){
    DBG(" in query_select. %u %u\n",max, status);
    json_t *ary = job_to_json(max, status);
    DBG(" json > json_dumps\n");
    char *ret = json_dumps(ary, JSON_INDENT(0));
    DBG("      > %s\n", ret);
    json_decref(ary);
    return ret;
}

static char *query_meta(){
    DBG(" in query_meta.\n");
    DBG(" meta: %s\n", meta_data);
    return strdup(meta_data);
}

static unsigned char *gzip_compress(const char* src, size_t src_size, size_t *out_size){
    DBG(" in gzip_compress() src_size:%ld.\n", src_size);
    z_stream zs = {0};
    unsigned char *buf = NULL;

    /* gzip形式を指定 */
    if (deflateInit2(&zs,
                Z_DEFAULT_COMPRESSION,
                Z_DEFLATED,
                15 + 16,       /* gzip */
                8,
                Z_DEFAULT_STRATEGY) != Z_OK)
        return NULL;
    uLong bound = deflateBound(&zs, src_size);
    DBG(" > bound:%ld.\n", bound);
    buf = malloc(bound);
    DBG(" > buf:%p.\n", buf); 
    if(!buf) goto ERROR;
    zs.next_in = (Bytef *)src;
    zs.avail_in = src_size;
    zs.next_out = buf;
    zs.avail_out = bound;

    int ret = deflate(&zs, Z_FINISH);
    DBG(" bound:%ld, total:%ld.\n", bound, zs.total_out);
    if (ret != Z_STREAM_END) goto ERROR;
    deflateEnd(&zs);
    *out_size = zs.total_out;
    return buf;
ERROR:
    free(buf);
    deflateEnd(&zs);
    return NULL; 
}

static char *gzip_decompress(char *src, size_t src_size, size_t *out_size){
    DBG(" in gzip_decompress() src_size:%ld.\n", src_size);
    z_stream zs = {0};
    char *buf = NULL;

    if (inflateInit2(&zs, 15 + 16) != Z_OK)
        return NULL;
    size_t capacity = src_size * 4;
    if (capacity < 4096)
        capacity = 4096;

    buf = malloc(capacity);
    if(!buf) goto ERROR;
    zs.next_in = (Bytef *)src;
    zs.avail_in = src_size;
    size_t total = 0;
    while(1){
        if (total == capacity) {
            capacity *= 2;
            char *tmp = realloc(buf, capacity);
            if(!tmp) goto ERROR;
            buf = tmp;
        }
        zs.next_out = (Bytef*)buf + total;
        zs.avail_out = capacity - total;
        int ret = inflate(&zs, Z_NO_FLUSH);
        total = capacity - zs.avail_out;
        if (ret == Z_STREAM_END)
            break;
        if(ret != Z_OK) goto ERROR;
    }
    inflateEnd(&zs);
    *out_size = total;
    char *tmp = realloc(buf, total+1);
    if(!tmp) goto ERROR;
    tmp[total] = '\0';
    return tmp;
ERROR:
    free(buf);
    inflateEnd(&zs);
    return NULL;
}

static enum MHD_Result http_response(struct MHD_Connection *connection, unsigned int status_code){
    enum MHD_Result ret;
    struct MHD_Response *resp;
    resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, status_code, resp);
    MHD_destroy_response(resp);
    return ret;
}

struct post_data {
    char *data;
    size_t len;
};

static enum MHD_Result ahc(void *cls,
                           struct MHD_Connection *c,
                           const char *url,
                           const char *method,
                           const char *version,
                           const char *upload_data,
                           size_t *upload_data_size,
                           void **con_cls)
{
    (void)cls; (void)url; (void)version;

    DBG("new request.\n");
    if(conf.debug){
        DBG(" > headers\n");
        _debug_print_header(c);
    }

    struct MHD_BasicAuthInfo *auth = MHD_basic_auth_get_username_password3(c);
    if (!auth || http_auth(auth->username, auth->password)){
        DBG(" > auth failed.\n");
        return http_response(c, MHD_HTTP_UNAUTHORIZED);
    }
    DBG(" > auth OK.\n");
    if(!method){
        DBG(" > method is null\n");
        return http_response(c, MHD_HTTP_BAD_REQUEST);
    }
    DBG(" > method: %s\n", method);

    if (strncmp(method, GET, sizeof GET) == 0){
        DBG(" > GET\n");
        const char *query = MHD_lookup_connection_value(c, MHD_GET_ARGUMENT_KIND,"query");
        const char *id    = MHD_lookup_connection_value(c, MHD_GET_ARGUMENT_KIND,"id");
        const char *status= MHD_lookup_connection_value(c, MHD_GET_ARGUMENT_KIND,"status");
        const char *max   = MHD_lookup_connection_value(c, MHD_GET_ARGUMENT_KIND,"max");
        Job_status s = (unsigned int)(~deleted);
        if(status){
            s = atoi(status);
        }
        uint32_t m = UINT32_MAX;
        if(max){
            m = atoll(max);
        }

        DBG(" > GET query=%s\n",query);
        char *dst = NULL;
        if(query && strcmp(query, "restart")==0){
            DBG(" > query restart id= %s, status=%d.\n", id, s);
            query_restart(id, s);
            return http_response(c, MHD_HTTP_OK);
        }
        if(query && strcmp(query, "cancel")==0){
            DBG(" > query cancel id= %s, status=%d.\n", id, s);
            query_cancel(id, s);
            return http_response(c, MHD_HTTP_OK);
        }
       if(query && strcmp(query, "delete")==0){
            DBG(" > query delete id= %s, status=%d.\n", id, s);
            query_delete(id, s);
            return http_response(c, MHD_HTTP_OK);
        }
        if(query && strcmp(query,"select")==0){
            DBG(" > query select max=%u,status=%d.\n", m, s);
            dst = query_select(m, s);
        }
        if(query && strcmp(query,"meta")==0){
            DBG(" > query meta.\n");
            dst = query_meta();
            DBG(" META: %s\n", dst);
        }

        if(dst){
            size_t dst_size = strlen(dst);
            unsigned char *gzip = NULL;
            size_t gzip_size = SIZE_MAX;
            const char *accept_encoding = 
                MHD_lookup_connection_value(c, MHD_HEADER_KIND, "Accept-Encoding");
            if(accept_encoding && strstr(accept_encoding, "gzip")){
                gzip = gzip_compress(dst, dst_size, &gzip_size);
            }

            enum MHD_Result ret;
            struct MHD_Response *resp;
            if(gzip_size < dst_size){
                resp = MHD_create_response_from_buffer(gzip_size, (void*)gzip, MHD_RESPMEM_MUST_FREE);
                MHD_add_response_header(resp, "Content-Type", "application/json");
                MHD_add_response_header(resp, "Content-Encoding", "gzip");
                free(dst);
            }else{
                resp = MHD_create_response_from_buffer(dst_size, (void*)dst, MHD_RESPMEM_MUST_FREE);
                MHD_add_response_header(resp, "Content-Type", "application/json");
                free(gzip);
            }
            MHD_add_response_header(resp, "Vary", "Accept-Encoding");
            ret = MHD_queue_response(c, MHD_HTTP_OK, resp);
            MHD_destroy_response(resp);
            return ret;
        }
        return http_response(c, MHD_HTTP_BAD_REQUEST);
    }

    DBG(" > post");
    if (*con_cls == NULL) {
        *con_cls = calloc(1, sizeof(struct post_data));
        if (!con_cls) return MHD_NO;
        return MHD_YES;
    }
    struct post_data *pd = *con_cls;

    if (*upload_data_size != 0) {
        char *p = realloc(pd->data, pd->len + *upload_data_size + 1);
        if (!p) return MHD_NO;

        pd->data = p;
        memcpy(pd->data + pd->len, upload_data, *upload_data_size);
        pd->len += *upload_data_size;
        pd->data[pd->len] = '\0';

        *upload_data_size = 0;
        return MHD_YES;
    }

    DBG(" > post packete comped. post:[%s]\n", pd->data);
    char *post = NULL;
    
    DBG(" > check is compressed.\n");
    const char *encoding =
        MHD_lookup_connection_value(c, MHD_HEADER_KIND, "Content-Encoding");
    DBG(" > encoding: %s\n", encoding);
    if(encoding && strstr(encoding,"gzip")){
        DBG(" > compressed!.\n");
        size_t _size;
        post = gzip_decompress(pd->data, pd->len, &_size);
        free(pd->data);
        free(pd);
        pd = NULL;
    }else{
        DBG(" > plane text.\n");
        post = pd->data;
        free(pd);
    }
    
    DBG(" > check post has data.\n");
    if( post == NULL ){
        printf("⚠ post is null. %s\n", post);
        return http_response(c, MHD_HTTP_BAD_REQUEST);
    }

    DBG(" > search script file. post:%s.\n", post);
    char *scriptfile = script_search(post);
    if(scriptfile == NULL){
        printf("⚠ donot match any scripts. %s\n", post);
    }

    DBG("add post: %s, script: %s.\n", post, scriptfile);
    int ret = job_register(post, scriptfile);
    free(post);
    if(ret){
        DBG(" error at adding.\n");
        return http_response(c, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }else{
        DBG("added.\n");
        return http_response(c, MHD_HTTP_ACCEPTED);
    }
}

static volatile sig_atomic_t _running = 1;
static int _sig;
static void sig_handler(int sig) {
    _running = 0;
    _sig = sig;
}

int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
 
    DBG("config_load()\n");
    config_load();
    if(conf.debug){
        setvbuf(stdout, NULL, _IONBF, 0);
    }
    DBG("conf.cert %.26s...\n", conf.cert);
    DBG("conf.key  %.26s...\n", conf.key);

    DBG("script_load()\n");
    script_load();
    DBG("job_load()\n");
    job_load();
    
    DBG("create pthread\n");
    pthread_t th;
    pthread_create(&th, NULL, worker, NULL);
    pthread_detach(th);
    DBG("start MHD daemon\n");
    char mes[256] = {0};
    struct MHD_Daemon *d = NULL;
    if (conf.cert && conf.key) {
        d = MHD_start_daemon(
                MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS,
                conf.port,
                NULL, NULL,
                &ahc, NULL,
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
                NULL, NULL,
                &ahc, NULL,
                MHD_OPTION_END
                );
        snprintf(mes, sizeof mes, "start http server. waiting at :%d .\n", conf.port);
    }

    if (!d) {
        printf("failed to start server.\n");
        if(strlen(mes)){
            fprintf(stderr, "%s", mes);
        }
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

    dprintf(STDOUT_FILENO, "=== Server Stoped ===\n");
    dprintf(STDOUT_FILENO, "signal: %d\n", _sig);
    dprintf(STDOUT_FILENO, "line  : %d\n", _line);
    void *buf[32];
    int n = backtrace(buf, 32);
    backtrace_symbols_fd(buf, n, STDOUT_FILENO);
  
    return 0;
}
