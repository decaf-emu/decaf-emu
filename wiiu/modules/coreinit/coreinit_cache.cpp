#include "coreinit.h"
#include "coreinit_cache.h"
#include "util.h"

void
DCInvalidateRange(p32<void> addr, uint32_t size)
{
}

void
DCFlushRange(p32<void> addr, uint32_t size)
{
}

void
DCStoreRange(p32<void> addr, uint32_t size)
{
}

void
DCFlushRangeNoSync(p32<void> addr, uint32_t size)
{
}

void
DCStoreRangeNoSync(p32<void> addr, uint32_t size)
{
}

void
DCZeroRange(p32<void> addr, uint32_t size)
{
   // TODO: Check align direction is correct!
   size = alignDown(size, 32);
   addr = make_p32<void>(alignUp(static_cast<uint32_t>(addr), 32));
   memset(addr, 0, size);
}

void
DCTouchRange(p32<void>addr, uint32_t size)
{
}

void
CoreInit::registerCacheFunctions()
{
   RegisterSystemFunction(DCInvalidateRange);
   RegisterSystemFunction(DCFlushRange);
   RegisterSystemFunction(DCStoreRange);
   RegisterSystemFunction(DCFlushRangeNoSync);
   RegisterSystemFunction(DCStoreRangeNoSync);
   RegisterSystemFunction(DCZeroRange);
   RegisterSystemFunction(DCTouchRange);
}