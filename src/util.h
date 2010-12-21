#ifndef _H_UTIL
#define _H_UTIL

#define CHECK(cond) if((cond) < 0) { \
    fprintf(stderr, "ERROR AT %s:%d\n", __FILE__, __LINE__); \
    return -1; }
#define COYAML_ASSERT(value) if(!(value)) { \
    fprintf(stderr, "COAYML: Assertion " #value \
        " at " __FILE__ ":%d failed\n", __LINE__); \
    errno = ECOYAML_ASSERTION_ERROR; \
    return -1; }

#endif // _H_UTIL
