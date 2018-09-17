#pragma once
#include "snduser2_axfx.h"
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXChorusExp
{
   UNKNOWN(0xA0);
};
CHECK_SIZE(AXFXChorusExp, 0xA0);

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorusExp> chorus);

BOOL
AXFXChorusExpInit(virt_ptr<AXFXChorusExp> chorus);

void
AXFXChorusExpShutdown(virt_ptr<AXFXChorusExp> chorus);

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorusExp> chorus);

BOOL
AXFXChorusExpSettings(virt_ptr<AXFXChorusExp> chorus);

BOOL
AXFXChorusExpSettingsUpdate(virt_ptr<AXFXChorusExp> chorus);

} // namespace cafe::snduser2
