#pragma once
#include <common/cbool.h>
#include <common/be_val.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_clipboard Clipboard
 * \ingroup coreinit
 * @{
 */

BOOL
OSCopyFromClipboard(void *buffer,
                    be_val<uint32_t> *size);

BOOL
OSCopyToClipboard(const void *buffer,
                  uint32_t size);

/** @} */

} // namespace coreinit
