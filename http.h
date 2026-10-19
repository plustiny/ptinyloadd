#pragma once
#include <microhttpd.h>

enum MHD_Result accept_policy(void *cls,
              const struct sockaddr *addr,
              socklen_t addrlen);
enum MHD_Result access_handler(void *cls,
                           struct MHD_Connection *c,
                           const char *url,
                           const char *method,
                           const char *version,
                           const char *upload_data,
                           size_t *upload_data_size,
                           void **con_cls);
