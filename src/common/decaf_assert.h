#pragma once
#include <string>
#include "platform.h"

#ifdef PLATFORM_WINDOWS

#define decaf_assert(x, m) \
   if (!(x)) { \
      assertFailed(__FILE__, __LINE__, m); \
      __debugbreak(); \
      abort(); \
   }

#else

#define decaf_assert(x, m) \
   if (!(x)) { \
      assertFailed(__FILE__, __LINE__, m); \
      __builtin_trap(); \
   }

#endif

#define decaf_check(x) \
   decaf_assert(x, #x)

#define decaf_abort(m) \
   decaf_assert(false, m)

void
assertFailed(const char *file,
             unsigned line,
             const std::string &message);
