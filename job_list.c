#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include "main.h"
#include "settings.h"
#include "job_list.h"
#include "script_list.h"
#include "utf8.h"

static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  job_cond  = PTHREAD_COND_INITIALIZER;

static int rand_int(){
    unsigned int x = ((unsigned)rand() << 16) ^ (unsigned)rand();
    return (int)x;
}

static uint32_t _id = 0;
static const char ALPHABET[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-";
static char *id_new(){
    uint32_t n = _id++;
    char buf[8];
    char out[8];
    if (n == 0) {
        out[0] = ALPHABET[0];
        out[1] = '\0';
        return strdup(out);
    }
    int i = 0;
    while (n > 0) {
        buf[i++] = ALPHABET[n % 64];
        n /= 64;
    }
    for (int j = 0; j < i; j++) {
        out[j] = buf[i - 1 - j];
    }
    out[i] = '\0';
    return strdup(out);
}

static void id_reset(){
    _id = 0;
}


static Job_t *job_head = NULL;
static Job_t *job_tail = NULL;

static void job_add(Job_t *x){
    if(job_tail){
        job_tail->next = x;
    }else{
        job_head = x;
    }
    job_tail = x;
}

static Job_t *job_free(Job_t *job){
    Job_t *next = NULL; 
    if(job != NULL){
        next = job->next;
    }
    job->status = none;
    FREE(job->id);
    FREE(job->post);
    FREE(job->out);
    FREE(job->err);
    FREE(job->scriptfile);
    FREE(job->scriptname);
    FREE(job);
    return next;
}

 void job_clean(){
    DBG(">> job_clean.");
    Job_t *job = job_head;
    while(job){
        if(job->status != deleted)
            break;
        job = job_free(job);
    }
    if(job == NULL){
        DBG("list is empty.");
        job_head = NULL;
        job_tail = NULL;
        id_reset();
        DBG("end.\n");
        return;
    }
    DBG(".");
    job_head = job;

    Job_t *tail = job_head;
    job = job_head->next;
    while(job){
        if(job->status != deleted){
            tail->next = job;
            tail = job;
            job = job->next;
        }else{
            job = job_free(job);
        }
    }
    job_tail = tail;
    DBG(".");
    job_tail->next = NULL;
    DBG("end\n");
}

int job_status_changed(){
    pthread_mutex_lock(&job_mutex);
    pthread_cond_signal(&job_cond);
    pthread_mutex_unlock(&job_mutex);
    return 0;
}

int job_register(char *post, script_t *script){
    pthread_mutex_lock(&job_mutex);
    Job_t *job = calloc(1, sizeof(Job_t));
    if(job==NULL)   return -1;
    job->id = id_new();
    if(post != NULL){
        job->post = strdup(post);
    }
    if(script != NULL){
        job->scriptfile = strdup(script->file);
        job->scriptname = strdup(script->name);
        job->status = unprocessed;
    }else{
        job_clear_buf(job);
        job->err = strdup("don't match any script.");
        job->status = canceled;
    }
    job->token = rand_int();
    job_add(job);
    pthread_cond_signal(&job_cond);
    pthread_mutex_unlock(&job_mutex);
    return 0;
}

Job_t *job_next(){
    pthread_mutex_lock(&job_mutex);
    Job_t *job = NULL;
    while(job == NULL){
        job = job_head;
        while(job){
            if(job->status == unprocessed)
                break;
            job = job->next;
        }
        if(job == NULL){
            job_clean();
            pthread_cond_wait(&job_cond, &job_mutex);
        }
    }
    pthread_mutex_unlock(&job_mutex);
    return job;
}

static char *json_str_val(json_t *obj, char *key){
        const char *s;
        s = json_string_value(json_object_get(obj, key));
        return s? strdup(s): NULL;
}

json_t *job_to_json(uint32_t max, Job_status status){
    DBG("job_to_json\n");
    json_error_t json_error;
    json_t *ary = json_array();
    Job_t *job = job_head;
    while(job){
        if(max-- == 0 )
            break;
        if(job->status & status){
            DBG_JOB(job);
            if(utf8_fix(job->out))  DBG("utf8 fixed\n");
            if(utf8_fix(job->err))  DBG("utf8 fixed\n");
            json_t *o = json_pack_ex(&json_error, 0,
                    "{s:s?, s:i, s:i, s:s?, s:s?, s:s?, s:s?, s:s?}",
                    "id", job->id,
                    "ec", job->ec,
                    "status", job->status,
                    "scriptfile", job->scriptfile,
                    "scriptname", job->scriptname,
                    "post", job->post,
                    "out", job->out,
                    "err", job->err);
            if(o == NULL)   DBG("job_to_json %s \n", json_error.text);
            json_array_append_new(ary, o);
        }        
        job = job->next;
    }
    return ary;
}
#undef SET_FIELD

#define JSON_ITEM(key, value) do{\
    json_object_set_new(o, #key, json_integer(value));\
}while(0);

json_t *job_metadata(){
    json_t *o = json_object();
    JOB_STATUS_LIST(JSON_ITEM);
    return json_pack("{s,o}","job_status", o);
}
#undef JSON_ITEM

void job_unload(){
    DBG("job_unload.\n");
    char pathname[1024];
    snprintf(pathname, sizeof(pathname), "%s/%s", conf.dl_dir, ABORT_FILE);
    
    char datetime[64];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", &tm);
    
    json_t *ary = job_to_json(UINT32_MAX, all);
    json_t *o = json_pack(
            "{s:s, s:i, s:o?}",
            "time", datetime,
            "_id", _id,
            "jobs", ary);
    json_dump_file(o, pathname, JSON_INDENT(1));
    json_decref(ary);
    eprintf("> write job list to %s .\n",pathname);

    Job_t *job = job_head;
    while(job){
        job = job_free(job);
    }
    job_head = NULL;
    job_tail = NULL;
}

void job_load(){
    srand((unsigned)time(NULL));
    char pathname[1024];
    snprintf(pathname, sizeof(pathname), "%s/%s", conf.dl_dir, ABORT_FILE);

    json_t *json = json_load_file(pathname, 0, NULL);
    _id = json_integer_value(json_object_get(json,"_id"));
    json_t *ary = json_object_get(json,"jobs");
    for (size_t i = 0; i < json_array_size(ary); i++) {
        json_t *obj = json_array_get(ary, i);
        Job_t *job = calloc(1, sizeof(Job_t));
        if(job == NULL){
            eprintf("❌ memory allocation failed at job_load().\n");
            return;
        }
        job->id = json_str_val(obj, "id");
        job->scriptfile = json_str_val(obj, "scriptfile");
        job->scriptname = json_str_val(obj, "scriptname");
        job->post = json_str_val(obj, "post");
        job->out = json_str_val(obj, "out");
        job->err = json_str_val(obj, "err");
        job->ec = json_integer_value(json_object_get(obj,"ec"));
        job->status = json_integer_value(json_object_get(obj,"status"));
        job->token = rand_int();
        job_add(job);
    }
}

void job_change_status(const char *id, Job_status status, Job_status running, Job_status waiting){
    DBG("job_change_status( id=%s, status=%d )\n", id, status);
    Job_t *job = job_head;
    while(job){
        DBG("> job->id=%s, job->status=%d.\n",job->id, job->status);
        if((id == NULL && (job->status & status))
                || (id != NULL && strncmp(job->id, id, strlen(id))==0)){
            DBG("> change it!\n");
            if(job->status == processing){
                Job_status expected = processing;
                atomic_compare_exchange_strong(&(job->status), &expected, running);
            }else{
                atomic_store(&(job->status), waiting);
            }
            if(job->status == unprocessed){
                job_clear_buf(job);
                job->ec = 0;
            }
        }
        job = job->next;
    }
    job_status_changed();
}

void job_clear_buf(Job_t *job){
    if(!job){
        eprintf(" job is null.\n");
    }
    if(job->out){
        memset(job->out, 0, sizeof(char) * JOB_LINE_LENGTH);
    }else{
        job->out = calloc(JOB_LINE_LENGTH, sizeof(char));
    }
    if(job->err){
        memset(job->err, 0, sizeof(char) * JOB_LINE_LENGTH);
    }else{
        job->err = calloc(JOB_LINE_LENGTH, sizeof(char));
    }
}

int job_auth(char *_pid, char *_token){
    errno = 0;
    pid_t pid = (pid_t)strtol(_pid, NULL, 10);
    if(errno) return false;
    int token = (int)strtol(_token, NULL, 10);
    if(errno) return false;
    Job_t *job = job_head;
    while(job){
        if(job->pid == pid && job->token == token)
            return true;
        job = job->next;
    }
    return false;
}
