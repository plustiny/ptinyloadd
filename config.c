#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
    FILE *fp = NULL;
    char *buf = NULL;
    if(!path)   goto ERROR;
    fp = fopen(path, "rb");
    if (!fp)    goto ERROR;
    
    size_t size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    if(errno)   goto ERROR;

    buf = malloc(size + 1);
    if (!buf)   goto ERROR;

    if (fread(buf, 1, size + 1, fp) != size) goto ERROR;

    buf[size] = '\0';
    fclose(fp);
    return buf;
ERROR:
    if(fp)  fclose(fp);
    if(buf) free(buf);
    return NULL;
}

void config_load(){
    conf.debug = getenv("DEBUG")? 1 : 0;
    char *port = getenv("PORT");
    if(port){
        conf.port = atoi(port);
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


