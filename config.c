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
    .script_dir = "./libexec/ptinyloadd",
    .nthread = 2, 
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

static int enabled(char *env){
    char *s = getenv(env);
    if( s && (s[0] == 'n' || s[0] == 'N' || s[0] == '0')){
        return 0;
    }
    return 1;
}

void config_load(){
    conf.debug = enabled("DEBUG");
    DBG("DEBUG: %d\n", conf.debug);
    char *port = getenv("PORT");
    if(port){
        conf.port = atoi(port);
    }
    DBG("PORT: %d\n", conf.port);
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
    if(getenv("SCRIPT_DIR"))
        conf.script_dir = getenv("SCRIPT_DIR");
    DBG("SCRIPT_DIR: %s\n", conf.script_dir);
    conf.cert = load_file(getenv("CERT"));
    DBG("conf.cert %.26s\n", conf.cert);
    conf.key = load_file(getenv("KEY"));
    DBG("conf.key  %.26s\n", conf.key);
    conf.force_https = enabled("FORCE_HTTPS");
    DBG("FORCE_HTTPS: %d\n", conf.force_https); 
    if (conf.cert && conf.key) {
        setenv("SCHEME", "https", 1);
    }else{
        setenv("SCHEME", "http", 1);
    }
    char *s = getenv("NTHREAD");
    if(s){
        conf.nthread = atoi(s);
    }
    DBG("NTHREAD: %d\n", conf.nthread);
    conf.no_auth_local = enabled("NO_AUTH_LOCAL");
    DBG("NO_AUTH_LOCAL %d\n", conf.no_auth_local);
   return;
}

void config_unload(){
    free(conf.cert);
    free(conf.key);
    return;
}


