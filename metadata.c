#define S(...)  #__VA_ARGS__

const char *meta_data = S(
    {
        "job_status" : {
        "0": "none",
        "1": "unprocessed",
        "2": "processing",
        "4": "finished",
        "8": "error",
        "16": "canceling",
        "32": "canceled",
        "64": "deleting",
        "128": "deleted",
        "255": "all" 
        }
    }
);
