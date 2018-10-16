#include "nn_olv.h"
#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::nn_olv
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   return 0;
}

static void
pure_virtual_called()
{
   coreinit::internal::OSPanic("nn_olv", 0, "__pure_virtual_called");
   coreinit::ghs_exit(6);
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   RegisterFunctionExportName("__pure_virtual_called", pure_virtual_called);
   registerDownloadedCommunityDataSymbols();
   registerDownloadedDataBaseSymbols();
   registerDownloadedPostDataSymbols();
   registerDownloadedTopicDataSymbols();
   registerInitSymbols();
   registerInitializeParamSymbols();
   registerUploadedDataBaseSymbols();
   registerUploadedPostDataSymbols();
}

} // namespace cafe::nn_olv
