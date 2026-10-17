#pragma once
#include <jansson.h>
#include <stdint.h>
#include <stdatomic.h>

#define JOB_LINE_LENGTH 512

typedef enum Job_status {
    all = (unsigned int)-1,
    none = 0,
    unprocessed = 1,
    processing = 2,
    finished = 4,
    error = 8,
    canceling = 16,
    canceled = 32,
    deleting = 64,
    deleted = 128,
} Job_status;

struct Job {
    char *id;
    char *post;
    char *out;
    char *err;
    char *scriptfile;
    int  ec;
    _Atomic Job_status status;
		struct Job *next;
};
typedef struct Job Job_t;

void job_load();
void job_unload();
int job_register(char *post, char *scriptfile);
Job_t *job_next();
json_t *job_to_json(uint32_t max, Job_status status);
void job_change_status(const char *id, Job_status status, Job_status proc, Job_status wait);
void job_clear_buf(Job_t *);
 
