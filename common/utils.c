#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* my_safe_malloc(size_t len, const char *funcname, int line, const char *file) {
    __u_char *ptr;
    if ((ptr = malloc(len)) == NULL) {
        fprintf(stderr, "%s:%s() line %d error while malloc %zu bytes \n", file, funcname, line, len);    
        exit(-1);
    }

    memset(ptr, 0, len);

    return (void*)ptr;
}

void my_safe_free(void *ptr, const char *funcname, int line, const char *file) {
    if (ptr == NULL) {
        fprintf(stderr, "%s:%s() line %d try to free a NULL pointer\n", file, funcname, line);
        exit(-1);
    }
    free(ptr);
}