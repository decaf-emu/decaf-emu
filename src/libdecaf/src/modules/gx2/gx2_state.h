#pragma once
#include "common/types.h"
#include "modules/coreinit/coreinit_time.h"
#include "common/be_val.h"

namespace gx2
{

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

namespace internal
{

bool isInited();

uint32_t getMainCoreId();

} // namespace internal

} // namespace gx2
