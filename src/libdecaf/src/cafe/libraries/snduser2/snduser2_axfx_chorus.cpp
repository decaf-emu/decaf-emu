#include "snduser2.h"
#include "snduser2_axfx_chorus.h"
#include "snduser2_axfx_chorusexp.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/sndcore2/sndcore2_config.h"

namespace cafe::snduser2
{

using namespace cafe::sndcore2;

/**
 * Converts the params from the AXFXChorus struct to the AXFXChorusExp params.
 */
static void
translateChorusUserParamsToExp(virt_ptr<AXFXChorus> chorus)
{
   // TODO: Implement translateChorusUserParamsToExp
   decaf_warn_stub();
}

int32_t
AXFXChorusGetMemSize(virt_ptr<AXFXChorus> chorus)
{
   return AXFXChorusExpGetMemSize(virt_addrof(chorus->chorusExp));
}

BOOL
AXFXChorusInit(virt_ptr<AXFXChorus> chorus)
{
   translateChorusUserParamsToExp(chorus);
   return AXFXChorusExpInit(virt_addrof(chorus->chorusExp));
}

BOOL
AXFXChorusShutdown(virt_ptr<AXFXChorus> chorus)
{
   AXFXChorusExpShutdown(virt_addrof(chorus->chorusExp));
   return TRUE;
}

void
AXFXChorusCallback(virt_ptr<AXFXBuffers> buffers,
                      virt_ptr<AXFXChorus> chorus)
{
   AXFXChorusExpCallback(buffers, virt_addrof(chorus->chorusExp));
}

BOOL
AXFXChorusSettings(virt_ptr<AXFXChorus> chorus)
{
   translateChorusUserParamsToExp(chorus);
   return AXFXChorusExpSettings(virt_addrof(chorus->chorusExp));
}

void
Library::registerAxfxChorusSymbols()
{
   RegisterFunctionExport(AXFXChorusGetMemSize);
   RegisterFunctionExport(AXFXChorusInit);
   RegisterFunctionExport(AXFXChorusShutdown);
   RegisterFunctionExport(AXFXChorusCallback);
   RegisterFunctionExport(AXFXChorusSettings);
}

} // namespace cafe::snduser2
