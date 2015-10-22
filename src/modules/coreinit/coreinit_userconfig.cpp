#include "coreinit.h"
#include "coreinit_userconfig.h"

IOHandle
UCOpen()
{
   return static_cast<IOHandle>(0);
}

void
UCClose(IOHandle handle)
{
}

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   for (auto i = 0; i < count; ++i) {
      auto &setting = settings[i];
      if (strcmp(setting.name, "cafe.language") == 0) {
         assert(setting.data_size == 4);
         *static_cast<uint32_t*>(setting.data.get()) = SCILanguage::English;
      } else {
         gLog->info("UCReadSysConfig unknown setting {} ({})", setting.name, setting.data_size);
      }
   }
   return IOError::OK;
}

void
CoreInit::registerUserConfigFunctions()
{
   RegisterKernelFunction(UCOpen);
   RegisterKernelFunction(UCClose);
   RegisterKernelFunction(UCReadSysConfig);
}
