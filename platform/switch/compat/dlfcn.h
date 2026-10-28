#pragma once

#include <stddef.h>

// No dynamic loading on Horizon homebrew: GDExtension libraries can't be opened.
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 4
#define RTLD_LOCAL 0
#define RTLD_NEXT ((void *)-1)

static inline void *dlopen(const char *, int) { return NULL; }
static inline void *dlsym(void *, const char *) { return NULL; }
static inline int dlclose(void *) { return 0; }
static inline const char *dlerror(void) { return "dynamic loading unsupported on Horizon"; }
