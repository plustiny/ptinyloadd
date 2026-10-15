#pragma once
#define DBG(...)    do { if (conf.debug) printf(__VA_ARGS__);} while (0)
#define FIRST_ARG(first, ...) first
