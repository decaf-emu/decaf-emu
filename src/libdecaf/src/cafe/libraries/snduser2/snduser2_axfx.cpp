#include "snduser2.h"
#include "snduser2_axfx.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

void
AXFXMultiChReverbCallback(virt_ptr<AXFXBuffers> buffers,
                          virt_ptr<AXFXReverbMulti> data,
                          virt_ptr<AXAuxCallbackData> auxData)
{
   decaf_warn_stub();
}

BOOL
AXFXMultiChReverbInit(virt_ptr<AXFXReverbMulti> reverb,
                      uint32_t unk,
                      AXFXSampleRate sampleRate)
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxSymbols()
{
   RegisterFunctionExport(AXFXMultiChReverbCallback);
   RegisterFunctionExport(AXFXMultiChReverbInit);
}

} // namespace cafe::snduser2
