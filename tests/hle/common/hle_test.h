#pragma once
#include <coreinit/debug.h>

#define test_assert(x) \
   if (!(x)) { \
      OSPanic(__FILE__, __LINE__, "assert(%s)", #x); \
   }

#define test_eq(x, y) \
   if (!((x) == (y))) { \
      OSPanic(__FILE__, __LINE__, "test_eq(%s, %s)", #x, #y); \
   }

#define test_gt(x, y) \
   if (!((x) > (y))) { \
      OSPanic(__FILE__, __LINE__, "test_gt(%s, %s)", #x, #y); \
   }

#define test_fail(x) \
   OSPanic(__FILE__, __LINE__, "Test failed: %s", x)

#define test_report(...) \
   OSReport(__VA_ARGS__)
