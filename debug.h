#pragma once
#include <stdio.h>

#define DBG(...)    do { if (conf.debug) fprintf(stderr, __VA_ARGS__);} while (0)
#define FIRST_ARG(first, ...) first
