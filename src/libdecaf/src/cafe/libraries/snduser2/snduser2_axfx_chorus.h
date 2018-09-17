#pragma once
#include "snduser2_axfx.h"
#include "snduser2_axfx_chorusexp.h"

#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct AXFXChorus
{
   be2_struct<AXFXChorusExp> chorusExp;
   UNKNOWN(0xC);
};
CHECK_SIZE(AXFXChorus, 0xAC);

int32_t
AXFXChorusGetMemSize(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXChorusInit(virt_ptr<AXFXChorus> chorus);

BOOL
AXFXChorusShutdown(virt_ptr<AXFXChorus> chorus);

void
AXFXChorusCallback(virt_ptr<AXFXBuffers> buffers,
                   virt_ptr<AXFXChorus> chorus);

BOOL
AXFXChorusSettings(virt_ptr<AXFXChorus> chorus);

} // namespace cafe::snduser2
