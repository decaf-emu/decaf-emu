#include "coreinit.h"
#include "coreinit_userconfig.h"

static const IOHandle
gUCHandle = 0x12345678;

IOHandle
UCOpen()
{
   return gUCHandle;
}

void
UCClose(IOHandle handle)
{
   assert(handle == gUCHandle);
}

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   if (handle != gUCHandle) {
      return IOError::Generic;
   }

   for (auto i = 0u; i < count; ++i) {
      auto &setting = settings[i];
      auto ptr8 = reinterpret_cast<uint8_t*>(setting.data.get());
      auto ptr32 = reinterpret_cast<uint32_t*>(setting.data.get());

      if (strcmp(setting.name, "cafe.cntry_reg") == 0) {
         assert(setting.dataType == UCDataType::Uint32);
         assert(setting.dataSize == 4);
         *ptr32 = SCICountryCode::USA;
      } else if (strcmp(setting.name, "cafe.eco") == 0) {
         assert(setting.dataType == UCDataType::Uint8);
         assert(setting.dataSize == 1);
         *ptr8 = 0; // TODO: Check value
      } else if (strcmp(setting.name, "cafe.eula_version") == 0) {
         assert(setting.dataType == UCDataType::Uint32);
         assert(setting.dataSize == 4);
         *ptr32 = 0; // TODO: Check value
      } else if (strcmp(setting.name, "cafe.fast_boot") == 0) {
         assert(setting.dataType == UCDataType::Uint32);
         assert(setting.dataSize == 4);
         *ptr32 = 0; // TODO: Check value
      } else if (strcmp(setting.name, "cafe.initial_launch") == 0) {
         assert(setting.dataType == UCDataType::Uint8);
         assert(setting.dataSize == 1);
         *ptr8 = 1; // TODO: Check value
      } else if (strcmp(setting.name, "cafe.language") == 0) {
         assert(setting.dataType == UCDataType::Uint32);
         assert(setting.dataSize == 4);
         *ptr32 = SCILanguage::English;
      } else if (strcmp(setting.name, "cafe.version") == 0) {
         assert(setting.dataType == UCDataType::Uint8);
         assert(setting.dataSize == 1);
         *ptr8 = 0; // TODO: Check value
      } else {
         gLog->info("UCReadSysConfig unknown setting {} (type: {} size: {})", setting.name, setting.dataType, setting.dataSize);
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
