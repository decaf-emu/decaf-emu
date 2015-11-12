#pragma once
#include "types.h"
#include "modules/coreinit/coreinit_time.h"
#include "utils/be_val.h"

namespace GX2InitAttrib
{
enum Value
{
   End = 0,
   CommandBufferPoolBase = 1,
   CommandBufferPoolSize = 2,
   ArgC = 7,
   ArgV = 8
};
}

void
GX2Init(be_val<uint32_t> *attributes);

void
GX2Shutdown();

void
GX2Flush();

namespace gx2
{

namespace internal
{

uint32_t getMainCoreId();

} // namespace internal

} // namespace gx2
