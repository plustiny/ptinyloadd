#pragma once
#include <jansson.h>
#include <stdint.h>
#include <stdatomic.h>
#include "script_list.h"

#define JOB_LINE_LENGTH 512

#define JOB_STATUS_LIST(X) \
    X(all, (uint8_t)-1) \
    X(none, 0) \
    X(unprocessed, 1) \
    X(processing, 2) \
    X(finished, 4) \
    X(error,  8) \
    X(canceling, 16) \
    X(canceled, 32) \
    X(deleting, 64) \
    X(deleted, 128) \
    X(terminated, finished|error|canceling|canceled) \

#define ENUM_ITEM(name, value) name = value,
typedef enum : uint8_t {
    JOB_STATUS_LIST(ENUM_ITEM)
} Job_status;
#undef ENUM_ITEM


struct Job {
    char *id;
    char *post;
    char *out;
    char *err;
    char *scriptfile;
    char *scriptname;
    int  ec;
    _Atomic Job_status status;
    pid_t   pid;
    int     token;
		struct Job *next;
};
typedef struct Job Job_t;

void job_load();
void job_unload();
int job_register(char *post, script_t *script);
Job_t *job_next();
json_t *job_metadata();
json_t *job_to_json(uint32_t max, Job_status status);
void job_change_status(const char *id, Job_status status, Job_status proc, Job_status wait);
void job_clear_buf(Job_t *);
int job_auth(char *pid, char *token); 
