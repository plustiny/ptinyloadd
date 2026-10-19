#pragma once
#include <regex.h>
#
struct script {
    char *file;
    char *name;
    char *desc;
    regex_t matcher;
    struct script *next;
};
typedef struct script script_t;

void script_load();
void script_unload();
script_t *script_search(char *);

