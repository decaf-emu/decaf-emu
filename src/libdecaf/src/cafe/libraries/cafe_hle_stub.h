#pragma once

#ifdef _MSC_VER
#define PRETTY_FUNCTION_NAME __FUNCSIG__
#else
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

#define decaf_warn_stub() \
   { \
      static bool warned = false; \
      if (!warned) { \
         cafe::hle::warnStubInvoked(PRETTY_FUNCTION_NAME); \
         warned = true; \
      } \
   }

namespace cafe::hle
{

void
warnStubInvoked(const char *name);

} // namespace cafe::hle
