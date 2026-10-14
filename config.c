#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "config.h"

Config conf = {
    .port = 1881,
    .name = "foo",
    .passwd = "bar",
    .dl_dir = "/tmp",
    .script_dir = "./libexec/ptinyloadd"
};

static char *load_file(const char *path){
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    
    size_t size;
    char *buf;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    if (fread(buf, 1, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }

    buf[size] = '\0';
    fclose(fp);
    return buf;
}

void config_load(){
    conf.debug = getenv("DEBUG")? 1 : 0;
    if(getenv("PORT")){
        conf.port = atoi(getenv("PORT"));
    }
    if(getenv("NAME"))  conf.name = getenv("NAME");
    char *pw = getenv("PASSWD");
    if(pw){
        conf.passwd = strdup(pw);
        size_t n = strlen(pw);
        memset((void*)pw, '0', n);
    }
    pw = getenv("PASSWD_MD5");
    if(pw){
        conf.passwd_md5 = strdup(pw);
        size_t n = strlen(pw);
        memset((void*)pw, '0', n);
    }
    if(getenv("DL_DIR")){
        conf.dl_dir = getenv("DL_DIR");
    }else{
        setenv("DL_DIR", conf.dl_dir, 0);
    }
    if(getenv("SCRIPT_DIR"))    conf.script_dir = getenv("SCRIPT_DIR");
    conf.cert = load_file(getenv("CERT"));
    conf.key = load_file(getenv("KEY"));
    conf.force_https = getenv("FORCE_HTTPS")? 1: 0;
    return;
}

void config_unload(){
    free(conf.cert);
    free(conf.key);
    return;
}


