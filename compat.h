#pragma once

// Cross-platform compatibility: malloc_usable_size (Linux) vs _msize (Windows)
#ifdef _WIN32
#include <malloc.h>
#define malloc_usable_size(ptr) _msize(ptr)
#else
#include <malloc.h>
#endif
