#pragma once
#include <stdio.h>
#include "job_list.h"

#define eprintf(...)    do { fprintf(stderr, __VA_ARGS__);} while(0)
#define eputc(c)        do{ fputc(c, stderr);} while(0)
#define DBG(...)        do { if (conf.debug) fprintf(stderr, __VA_ARGS__);} while (0)
#define DBG_JOB(job)    do{\
        DBG("[id]%s, [stat]%d, [ec]%d  [post]%.5s, [f]%s, [n]%s\n, [out]%.5s\n, [err]%.5s\n", \
        job->id, job->status, job->ec, job->post, job->scriptfile, job->scriptname, job->out, job->err); \
    }while(0)
#define DBG_IF(b, ...)  do{\
        if(b) DBG(__VA_ARGS__);\
    }while(0) 

