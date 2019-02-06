#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

void
OSGetCodegenVirtAddrRange(virt_ptr<virt_addr> outAddress,
                          virt_ptr<uint32_t> outSize);

} // namespace cafe::coreinit
