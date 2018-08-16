#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_clipboard Clipboard
 * \ingroup coreinit
 * @{
 */

BOOL
OSCopyFromClipboard(virt_ptr<void> buffer,
                    virt_ptr<uint32_t> size);

BOOL
OSCopyToClipboard(virt_ptr<const void> buffer,
                  uint32_t size);

/** @} */

} // namespace cafe::coreinit
