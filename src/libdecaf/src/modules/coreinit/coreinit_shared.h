#pragma once
#include "coreinit_enum.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/cbool.h>
#include <cstdint>

namespace coreinit
{

BOOL
OSGetSharedData(OSSharedDataType type,
                uint32_t unk_r4,
                be_ptr<uint8_t> *addr,
                be_val<uint32_t> *size);

namespace internal
{

void
loadSharedData();

} // namespace internal

} // namespace coreinit
