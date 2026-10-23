#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "main.h"
#include "settings.h"

Settings conf = {
    .debug  = false,
    .port   = 1881,
    .lport  = 1882, 
    .nthread = 2, 
    .uname  = "foo",
    .passwd = "bar",
    .dl_dir = "/var/tmp",
    .script_dir = "./libexec/ptinyloadd",
};

static char *load_file(const char *path){
    FILE *fp = NULL;
    char *buf = NULL;
    if(!path)   goto ERROR;
    fp = fopen(path, "rb");
    if (!fp)    goto ERROR;
   
    errno = 0;
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
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

static int isenabled(char *env){
    char *s = getenv(env);
    if( s && (s[0] == 'n' || s[0] == 'N' || s[0] == '0')){
        return 0;
    }
    return 1;
}

static int envtoi(char *env, int def){
    char *s = getenv(env);
    if(!s)  return def;
    errno = 0;
    int ret = (int)strtol(s, NULL, 10);
    if(!errno)
        return ret;
    return def;
}

static char* envstr(char *env, char *def){
    char *s = getenv(env);
    if(s){
        return s;
    }else{
        return def;
    }
}

static char *get_and_clear(char *env, char *def){
    char *s = getenv(env);
    if(!s)  return def;
    char *ret = strdup(s);
    size_t n = strlen(s);
#if defined(__STDC_LIB_EXT1__)
    memset_explicit(s, 0, n);
#else
    explicit_bzero(s, n);
#endif
    unsetenv(env);
    return ret;
}

void settings_load(){
    conf.debug          = isenabled("DEBUG");
    conf.port           = envtoi("PORT", conf.port);
    conf.lport          = envtoi("LOCAL_PORT", conf.lport);
    conf.force_https    = isenabled("FORCE_HTTPS");
    conf.no_local_auth  = isenabled("NO_LOCAL_AUTH");
    conf.nthread        = envtoi("NTHREAD", conf.nthread); 
    conf.uname          = envstr("NAME", conf.uname);
    conf.passwd         = get_and_clear("PASSWD", conf.passwd);
    conf.passwd_md5     = get_and_clear("PASSWD_MD5", conf.passwd_md5);
    conf.dl_dir         = envstr("DL_DIR", conf.dl_dir);
    setenv("DL_DIR", conf.dl_dir, 0);
    conf.script_dir     = envstr("SCRIPT_DIR", conf.script_dir);
    conf.cert           = load_file(getenv("CERT"));
    conf.key            = load_file(getenv("KEY"));
    
    DBG("DEBUG: %d\n", conf.debug);
    DBG("PORT: %d\n", conf.port);
    DBG("LPORT: %d\n", conf.lport);
    DBG("FORCE_HTTPS: %d\n", conf.force_https); 
    DBG("NO_LOCAL_AUTH %d\n", conf.no_local_auth);
    DBG("NTHREAD: %d\n", conf.nthread);
    DBG("DL_DIR: %s\n", conf.dl_dir);
    DBG("SCRIPT_DIR: %s\n", conf.script_dir);
    DBG("CERT %.26s\n", conf.cert);
    DBG("KEY  %.26s\n", conf.key);
   return;
}

void settings_unload(){
    FREE(conf.passwd);
    FREE(conf.passwd_md5);
    FREE(conf.cert);
    FREE(conf.key);
    return;
}

