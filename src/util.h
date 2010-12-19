#ifndef _H_UTIL
#define _H_UTIL

#define CHECK(cond) if((cond) < 0) { \
    fprintf(stderr, "ERROR AT %s:%d\n", __FILE__, __LINE__); \
    return -1; }

#endif // _H_UTIL
