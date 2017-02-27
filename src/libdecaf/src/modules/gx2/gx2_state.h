#pragma once
#include "gx2_enum.h"
#include "modules/coreinit/coreinit_time.h"

#include <common/be_val.h>
#include <cstdint>

namespace gx2
{

void
GX2Init(be_val<uint32_t> *attributes);

void
GX2Shutdown();

void
GX2Flush();

namespace internal
{

void
enableStateShadowing();

void
disableStateShadowing();

bool
isInitialised();

uint32_t
getMainCoreId();

void
setMainCore();

} // namespace internal

} // namespace gx2
