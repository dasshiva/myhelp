#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <stdlib.h>

#define LOG(...) { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); }
#define fatal(...) { LOG("FATAL ERROR: " __VA_ARGS__); exit(1); }
#define warn(...) LOG("WARNING: " __VA_ARGS__);

#ifdef DEBUG
#define info(...) LOG("INFORMATION: " __VA_ARGS__);
#else
#define info(...) 
#endif

#endif
