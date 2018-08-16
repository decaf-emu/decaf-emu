#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

void
GX2Init(virt_ptr<GX2InitAttrib> attributes);

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

} // namespace cafe::gx2
