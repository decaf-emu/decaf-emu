#pragma once
#include <coreinit/debug.h>

#define test_assert(x) \
   if (!(x)) { \
      OSPanic(__FILE__, __LINE__, "assert(%s)", #x); \
   }

#define test_fail(x) \
   OSPanic(__FILE__, __LINE__, "Test failed: %s", x)

#define test_report(...) \
   OSReport(__VA_ARGS__)
