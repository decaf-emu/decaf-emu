#pragma once
#include <common/log.h>

#ifdef _MSC_VER
#define PRETTY_FUNCTION_NAME __FUNCSIG__
#else
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#define decaf_warn_stub() \
   { \
      static bool warned = false; \
      if (!warned) { \
         gLog->warn("Application invoked stubbed function `{}`", PRETTY_FUNCTION_NAME); \
         warned = true; \
      } \
   }
