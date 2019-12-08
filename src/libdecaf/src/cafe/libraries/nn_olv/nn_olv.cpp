#include "nn_olv.h"
#include "cafe/libraries/coreinit/coreinit_ghs.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"

namespace cafe::nn_olv
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

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
