#pragma once
#include "cafe/kernel/cafe_kernel_context.h"

namespace cafe::coreinit
{

using OSContext = cafe::kernel::Context;
CHECK_SIZE(OSContext, 0x320);

void
OSInitContext(virt_ptr<OSContext> context,
              virt_addr nia,
              virt_addr stackBase);

void
OSSetCurrentContext(virt_ptr<OSContext> context);

void
OSSetCurrentFPUContext(uint32_t unk);

void
OSSetCurrentUserContext(virt_ptr<OSContext> context);

} // cafe::coreinit
