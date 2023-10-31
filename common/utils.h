#pragma once
#include <sys/types.h>

#define safe_malloc(x) my_safe_malloc(x, __FUNCTION__, __LINE__, __FILE__)
void* my_safe_malloc(size_t len, const char*, int, const char*);

#define safe_free(x) my_safe_free(x, __FUNCTION__, __LINE__, __FILE__)
void my_safe_free(void*, const char*, int, const char*);