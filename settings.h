#pragma once

typedef struct {
    int debug;
    int port;
    int lport;
    int force_https;
    int no_local_auth;
    int nthread;
    char *uname;
    char *passwd;
    char *passwd_md5;
    char *dl_dir;
    char *script_dir;
    char *cert;
    char *key;
} Settings;
extern Settings conf ;

void settings_load();
void settings_unload();

