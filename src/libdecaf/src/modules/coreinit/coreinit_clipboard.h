#pragma once

#include <cstdint>
#include <common/cbool.h>
#include <common/be_val.h>

namespace coreinit
{

BOOL
OSCopyFromClipboard(void *buffer,
                    be_val<uint32_t> *size);

BOOL
OSCopyToClipboard(const void *buffer,
                  uint32_t size);

} // namespace coreinit
