#pragma once

typedef struct Config {
    int debug;
    int port;
    char *name;
    char *passwd;
    char *passwd_md5;
    char *dl_dir;
    char *script_dir;
    char *cert;
    char *key;
    int force_https;
    int nthread;
    int no_auth_local;
} Config;
extern Config conf;

void config_load();
void config_unload();

