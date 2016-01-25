#pragma once
#include "types.h"

namespace coreinit
{

static const uint32_t CoreCount = 3;

uint32_t
OSGetCoreCount();

uint32_t
OSGetCoreId();

uint32_t
OSGetMainCoreId();

BOOL
OSIsMainCore();

} // namespace coreinit
