#pragma once
#include "systemtypes.h"

void
DCInvalidateRange(p32<void> addr, uint32_t size);

void
DCFlushRange(p32<void> addr, uint32_t size);

void
DCStoreRange(p32<void> addr, uint32_t size);

void
DCFlushRangeNoSync(p32<void> addr, uint32_t size);

void
DCStoreRangeNoSync(p32<void> addr, uint32_t size);

void
DCZeroRange(p32<void> addr, uint32_t size);

void
DCTouchRange(p32<void>addr, uint32_t size);
