#include "gx2.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::gx2
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerApertureSymbols();
   registerClearSymbols();
   registerContextStateSymbols();
   registerCounterSymbols();
   registerDebugSymbols();
   registerDisplaySymbols();
   registerDisplayListSymbols();
   registerDrawSymbols();
   registerEventSymbols();
   registerFetchShadersSymbols();
   registerFormatSymbols();
   registerMemorySymbols();
   registerQuerySymbols();
   registerRegistersSymbols();
   registerSamplerSymbols();
   registerShadersSymbols();
   registerStateSymbols();
   registerSurfaceSymbols();
   registerSwapSymbols();
   registerTempSymbols();
   registerTessellationSymbols();
   registerTextureSymbols();
   registerGx2rBufferSymbols();
   registerGx2rDisplayListSymbols();
   registerGx2rDrawSymbols();
   registerGx2rMemorySymbols();
   registerGx2rResourceSymbols();
   registerGx2rShadersSymbols();
   registerGx2rSurfaceSymbols();
}

} // namespace cafe::gx2
