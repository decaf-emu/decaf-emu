#include "coreinit.h"
#include "coreinit_memory.h"
#include "memory.h"

p32<void>
OSBlockMove(p32<void> dst, p32<const void> src, size_t size, BOOL flush)
{
   std::memmove(dst, src, size);
   return dst;
}

static p32<void>
coreinit_memcpy(p32<void> dst, p32<const void> src, size_t size)
{
   std::memcpy(dst, src, size);
   return dst;
}

static p32<void>
coreinit_memset(p32<void> dst, int val, size_t size)
{
   std::memset(dst, val, size);
   return dst;
}

void
CoreInit::registerMemoryFunctions()
{
   RegisterSystemFunction(OSBlockMove);
   RegisterSystemFunctionName("memset", coreinit_memset);
   RegisterSystemFunctionName("memcpy", coreinit_memcpy);
}
