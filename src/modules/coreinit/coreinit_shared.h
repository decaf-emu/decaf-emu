#pragma once
#include "types.h"
#include "coreinit_enum.h"
#include "utils/be_val.h"
#include "utils/virtual_ptr.h"

namespace coreinit
{

BOOL
OSGetSharedData(OSSharedDataType type, uint32_t, be_ptr<uint8_t> *addr, be_val<uint32_t> *size);

} // namespace coreinit
