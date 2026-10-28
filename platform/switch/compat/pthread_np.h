#pragma once

// Horizon's pthreads cannot name threads; thread_posix.cpp's BSD-style call becomes a no-op.
#include <pthread.h>

static inline void pthread_set_name_np(pthread_t, const char *) {}
