#ifndef _H_UTIL
#define _H_UTIL

#define CHECK(cond) if((cond) < 0) { \
    return -1; }
#define COYAML_ASSERT(value) if(!(value)) { \
    fprintf(stderr, "COYAML: Assertion " #value \
        " at " __FILE__ ":%d failed\n", __LINE__); \
    errno = ECOYAML_ASSERTION_ERROR; \
    return -1; }

#endif // _H_UTIL
