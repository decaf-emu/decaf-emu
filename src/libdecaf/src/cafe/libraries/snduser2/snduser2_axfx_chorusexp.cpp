#include "snduser2.h"
#include "snduser2_axfx_chorusexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

int32_t
AXFXChorusExpGetMemSize(virt_ptr<AXFXChorusExp> chorus)
{
   return 12 * (AXGetInputSamplesPerSec() / 10);
}

BOOL
AXFXChorusExpInit(virt_ptr<AXFXChorusExp> chorus)
{
   decaf_warn_stub();
   return TRUE;
}

void
AXFXChorusExpShutdown(virt_ptr<AXFXChorusExp> chorus)
{
   decaf_warn_stub();
}

void
AXFXChorusExpCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorusExp> chorus)
{
   decaf_warn_stub();
}

BOOL
AXFXChorusExpSettings(virt_ptr<AXFXChorusExp> chorus)
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXChorusExpSettingsUpdate(virt_ptr<AXFXChorusExp> chorus)
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxChorusExpSymbols()
{
   RegisterFunctionExport(AXFXChorusExpGetMemSize);
   RegisterFunctionExport(AXFXChorusExpInit);
   RegisterFunctionExport(AXFXChorusExpShutdown);
   RegisterFunctionExport(AXFXChorusExpCallback);
   RegisterFunctionExport(AXFXChorusExpSettings);
   RegisterFunctionExport(AXFXChorusExpSettingsUpdate);
}

} // namespace cafe::snduser2
