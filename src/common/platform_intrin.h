#pragma once

#if defined(_MSC_VER) || defined(__SSE3__)
#define PLATFORM_HAS_SSE3
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
