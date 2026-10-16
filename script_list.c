#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <regex.h>
#include "debug.h"
#include "config.h"
#include "script_list.h"

#define KEY_NAME "name"
#define KEY_DESC "description"
#define KEY_MATCH "match"

// 0-9 < A-Z < a-z
static int asc_order(const void *a, const void *b){
    return strcmp(*(const char **)a, *(const char **)b);
}

static char *trim(char *s){
    while (*s && isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s) -1;
    while(e > s && isspace((unsigned char)*e)) e--;
    *(e + 1) = '\0';
    return s;
}

struct script {
    char *scriptfile;
    char *name;
    char *desc;
    regex_t matcher;
    struct script *next;
};
typedef struct script script_t;

static script_t *script_head = NULL;
static script_t *script_tail = NULL;

static script_t *script_free(script_t *x){
    script_t *next = NULL;
    if(x){
        next = x->next;
    }
    free(x->scriptfile);
    free(x->name);
    free(x->desc);
    regfree(&x->matcher);
    free(x);
    return next;
}

static void script_add(script_t *x){
    if(script_tail){
        script_tail->next = x;
    }else{
        script_head = x;
    }
    script_tail = x;
}

void script_load(){
    DIR *dir = opendir(conf.script_dir);
    if(!dir){
        printf("no such directory.\n");
        return;
    }
    char *files[1024] ={0};
    int nfiles = 0;
    struct dirent *ent;
    while((ent = readdir(dir)) != NULL){
        if(ent->d_name[0] == '.') continue;
        files[nfiles++] = strdup(ent->d_name);
    }
    closedir(dir);

    qsort(files, nfiles, sizeof(char*), asc_order);

    const char *meta_pat = "^[[:space:]]*#[[:space:]]*@([[:alnum:]]+)[[:space:]]+\"?([^\"]+)\"?[[:space:]]*$";
    regex_t meta_reg;
    if (regcomp(&meta_reg, meta_pat, REG_EXTENDED) != 0) {
        printf("❌ ERROR! regcomp meta_pat.\n");
        return;
    }
    for(int i = 0; i < nfiles; i++){
        FILE *f = NULL;
        script_t *e = NULL;
        char *file = files[i];
        char path[1024];
        char line[1024];
        e = calloc(1, sizeof(script_t));
        if(e == NULL){
            printf("❌ ERROR! memory allocation failed at script_load().\n");
            goto ERROR;
        }
        e->scriptfile = file;
        snprintf(path, sizeof(path), "%s/%s", conf.script_dir, file);
        f = fopen(path, "rb");
        if(!f){
            printf("%s is not file. SKIP\n", file);
            goto ERROR;
        }
        while (fgets(line, sizeof line, f)) {
            DBG("file: %s, line: %s", file, line);
            regmatch_t match[3];
            if(regexec(&meta_reg, line, 3, match, 0))   continue;
            
            char *key = line + match[1].rm_so;
            *(line + match[1].rm_eo) = '\0';
            char *value = line + match[2].rm_so;
            *(line + match[2].rm_eo) = '\0';
            value = trim(value);
            DBG("key:value = [%s]:[%s]\n", key, value);
            
            if(strncmp(key, KEY_NAME, sizeof KEY_NAME) == 0){
                e->name = strdup(value);
            }
            if(strncmp(key, KEY_DESC, sizeof KEY_DESC) == 0){
                e->desc = strdup(value);
            }
            if(strncmp(key, KEY_MATCH, sizeof KEY_MATCH) == 0){
                DBG("convert %s to regex\n", value);
                int err = regcomp(&(e->matcher), value, REG_EXTENDED);
                DBG("regcomp err = %d\n", err);
                if(err){
                    char buf[256];
                    printf("❌ ERROR! invalid regex pattern in %s.\n", file);
                    regerror(err, &(e->matcher), buf, sizeof(buf));
                    printf("  %s\n", buf);
                    printf("  SKIPED.\n");
                    continue;
                }
            }
        }
        if(f)   fclose(f);
        script_add(e);
        printf("add script. name: %s, file: %s\n", e->name, e->scriptfile);
        continue;
ERROR:
        if(f)   fclose(f);
        if(e)   free(e);
    }
    return;
}

void script_unload(){
    script_t *e = script_head;
    while(e){
        e = script_free(e);
    }
}

char *script_search(char *post){
    DBG(" in script_search\n");
    script_t *e = script_head;
    while(e){
        DBG(" > checking script %s\n", e->name);
        int ret = regexec(&(e->matcher), post, 0, NULL, 0);
        if(!ret){
            DBG(" > found %s\n", e->name);
            return strdup(e->scriptfile);
        }
        e = e->next;
    };
    return NULL;
}

