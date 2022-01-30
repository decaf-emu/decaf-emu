#include "nn_boss.h"

namespace cafe::nn_boss
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

   registerAlmightyStorageSymbols();
   registerClientSymbols();
   registerDataNameSymbols();
   registerNbdlTaskSettingSymbols();
   registerNetTaskSettingSymbols();
   registerPlayLogUploadTaskSettingSymbols();
   registerPlayReportSettingSymbols();
   registerRawUlTaskSettingSymbols();
   registerStorageSymbols();
   registerTaskSymbols();
   registerTaskIdSymbols();
   registerTaskSettingSymbols();
   registerTitleSymbols();
   registerTitleIdSymbols();
}

} // namespace cafe::nn_boss
