#include "coreinit.h"
#include "coreinit_userconfig.h"
#include "utils/bitutils.h"

struct UCConfigEntry
{
   std::string name;
   UCDataType type;
   uint32_t value;
};

static std::vector<UCConfigEntry>
gUserConfig = {
   { "cafe",                  UCDataType::Group,   0 },
   { "cafe.cntry_reg",        UCDataType::Uint32,  SCICountryCode::USA },
   { "cafe.eco",              UCDataType::Uint8,   0 },
   { "cafe.eula_agree",       UCDataType::Uint8,   1 },
   { "cafe.eula_version",     UCDataType::Uint32,  0 },
   { "cafe.fast_boot",        UCDataType::Uint8,  0 },
   { "cafe.initial_launch",   UCDataType::Uint8,   0 },
   { "cafe.language",         UCDataType::Uint32,  SCILanguage::English },
   { "cafe.version",          UCDataType::Uint16,   0 },
};

static const IOHandle
gUCHandle = 0x12345678;

static bool
readSetting(UCSysConfig &setting)
{
   for (auto &entry : gUserConfig) {
      if (entry.name.compare(setting.name) != 0) {
         continue;
      }

      if (entry.type != setting.dataType) {
         gLog->error("Mismatch userconfig data type for {}, found {} expected {}", entry.name, setting.dataType, entry.type);
      }

      if (setting.data) {
         switch (entry.type) {
         case UCDataType::Uint8:
            assert(setting.dataSize == 1);
            *reinterpret_cast<uint8_t*>(setting.data.get()) = entry.value;
            break;
         case UCDataType::Uint16:
            assert(setting.dataSize == 2);
            *reinterpret_cast<uint16_t*>(setting.data.get()) = entry.value;
            break;
         case UCDataType::Uint32:
            assert(setting.dataSize == 4);
            *reinterpret_cast<uint32_t*>(setting.data.get()) = entry.value;
            break;
         default:
            throw std::logic_error(fmt::format("Invalid type for user config entry {}", entry.name));
         }
      }

      return true;
   }

   return false;
}

static bool
writeSetting(UCSysConfig &setting)
{
   for (auto &entry : gUserConfig) {
      if (entry.name.compare(setting.name) != 0) {
         continue;
      }

      if (!setting.data) {
         entry.value = 0;
      } else {
         if (entry.type != setting.dataType) {
            gLog->error("Mismatch userconfig data type for {}, found {} expected {}", entry.name, setting.dataType, entry.type);
         }

         switch (entry.type) {
         case UCDataType::Uint8:
            assert(setting.dataSize == 1);
            entry.value = *reinterpret_cast<uint8_t*>(setting.data.get());
            break;
         case UCDataType::Uint16:
            assert(setting.dataSize == 2);
            entry.value = *reinterpret_cast<uint16_t*>(setting.data.get());
            break;
         case UCDataType::Uint32:
            assert(setting.dataSize == 4);
            entry.value = *reinterpret_cast<uint32_t*>(setting.data.get());
            break;
         default:
            throw std::logic_error(fmt::format("Invalid type for user config entry {}", entry.name));
         }
      }

      return true;
   }

   return false;
}

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

      gLog->debug("UCReadSysConfig read setting {} (type: {} size: {})", setting.name, setting.dataType, setting.dataSize);

      if (!readSetting(setting)) {
         gLog->warn("UCReadSysConfig unknown setting {} (type: {} size: {})", setting.name, setting.dataType, setting.dataSize);
      }
   }

   return IOError::OK;
}

IOError
UCWriteSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   if (handle != gUCHandle) {
      return IOError::Generic;
   }

   for (auto i = 0u; i < count; ++i) {
      auto &setting = settings[i];
      uint32_t value = 0;

      if (setting.data) {
         value = *reinterpret_cast<uint32_t *>(setting.data.get());
         value &= make_bitmask<uint32_t>(setting.dataSize * 8);
      }

      gLog->debug("UCWriteSysConfig write setting {} = {} (type: {} size: {})", setting.name, value, setting.dataType, setting.dataSize);

      if (!writeSetting(setting)) {
         auto type = UCDataType::Uint32;

         if (setting.dataSize == 1) {
            type = UCDataType::Uint8;
         } else if (setting.dataSize == 2) {
            type = UCDataType::Uint16;
         }

         gUserConfig.push_back({ setting.name, type, 0 });
         writeSetting(setting);
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
   RegisterKernelFunction(UCWriteSysConfig);
}
