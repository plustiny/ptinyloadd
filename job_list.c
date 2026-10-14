#include <string.h>
#include <pthread.h>
#include "debug.h"
#include "config.h"
#include "job_list.h"

#define ABORT_FILE  "ptinyloadd_abort.json"

static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  job_cond  = PTHREAD_COND_INITIALIZER;

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
    DBG(" in job_free()\n");
    Job_t *next = NULL; 
    if(job != NULL){
        next = job->next;
    }
    job->status = none;
    free(job->id);
    free(job->post);
    free(job->line);
    free(job->scriptfile);
    free(job);
    DBG(" out job_free()\n");
    return next;
}

static void job_clean(){
    Job_t *job = job_head;
    while(job){
        if(job->status != deleted)
            break;
        job = job_free(job);
    }
    if(job == NULL){
        job_head = NULL;
        job_tail = NULL;
        id_reset();
        return;
    }
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
    job_tail->next = NULL;
}

int job_register(char *post, char *scriptfile){
    pthread_mutex_lock(&job_mutex);
    Job_t *job = calloc(1, sizeof(Job_t));
    if(job==NULL)   return -1;
    job->id = id_new();
    if(post != NULL){
        job->post = strdup(post);
    }
    if(scriptfile != NULL){
        job->scriptfile = strdup(scriptfile);
        job->status = unprocessed;
    }else{
        job_clear_line(job);
        job->line = strdup("don't match any script.");
        job->status = canceled;
    }
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
    json_error_t json_error;
    json_t *ary = json_array();
    Job_t *job = job_head;
    while(job){
        if(max-- == 0 )
            break;
        if(job->status & status){
            json_t *o = json_pack_ex(&json_error, 0,
                    "{s:s?, s:s?, s:s?, s:s?, s:i, s:i}",
                    "id", job->id,
                    "script", job->scriptfile,
                    "post", job->post,
                    "line", job->line,
                    "err", job->err,
                    "status", job->status);
            if(o == NULL)   DBG("json error %s \n", json_error.text);
            json_array_append_new(ary, o);
        }        
        job = job->next;
    }
    return ary;
}

void job_unload(){
    DBG(" in job_unload.\n");
    char pathname[1024];
    snprintf(pathname, sizeof(pathname), "%s/%s", conf.dl_dir, ABORT_FILE);
    
    char datetime[64];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", &tm);
    
    json_t *ary = job_to_json(UINT32_MAX, all);
    json_t *o = json_pack(
            "{s:s, s:i, s:o}",
            "time", datetime,
            "_id", _id,
            "jobs", ary);
    json_dump_file(o, pathname, JSON_INDENT(1));
    json_decref(ary);
    DBG(" > write job list to %s.\n",pathname);

    Job_t *job = job_head;
    while(job){
        job = job_free(job);
    }
    job_head = NULL;
    job_tail = NULL;
}

void job_load(){
    char pathname[1024];
    snprintf(pathname, sizeof(pathname), "%s/%s", conf.dl_dir, ABORT_FILE);

    json_t *json = json_load_file(pathname, 0, NULL);
    _id = json_integer_value(json_object_get(json,"_id"));
    json_t *ary = json_object_get(json,"jobs");
    for (size_t i = 0; i < json_array_size(ary); i++) {
        json_t *obj = json_array_get(ary, i);
        Job_t *job = calloc(1, sizeof(Job_t));
        if(job == NULL){
            printf("❌ memory allocation failed at job_load().\n");
            return;
        }
        job->id = json_str_val(obj, "id");
        job->scriptfile = json_str_val(obj, "script");
        job->post = json_str_val(obj, "post");
        job->line = json_str_val(obj, "line");
        job->err = json_integer_value(json_object_get(obj,"err"));
        job->status = json_integer_value(json_object_get(obj,"status"));
        job_add(job);
    }
}

void job_change_status(const char *id, Job_status status, Job_status proc, Job_status wait){
    DBG(" > lets change id=%s, status=%d.\n", id, status);
    Job_t *job = job_head;
    while(job){
        DBG(" > job->id=%s, job->status=%d.\n",job->id, job->status);
        if((id == NULL && (job->status & status)) || strcmp(job->id, id)==0){
            DBG(" > change it!\n");
            if(job->status == processing){
                job->status = proc;
            }else{
                job->status = wait;
            }
        }
        job = job->next;
    }
}

void job_clear_line(Job_t *job){
    if(!job){
        DBG(" job is null.\n");
    }
    if(job->line){
        memset(job->line, 0, sizeof(char) * JOB_LINE_LENGTH);
    }else{
        job->line = calloc(JOB_LINE_LENGTH, sizeof(char));
    }
}

