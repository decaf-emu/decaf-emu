#include "coreinit.h"
#include "coreinit_enum_string.h"
#include "coreinit_userconfig.h"
#include <common/bitutils.h>
#include <common/decaf_assert.h>

namespace coreinit
{

struct UCConfigEntry
{
   std::string name;
   UCDataType type;
   std::vector<uint8_t> data;
};

static std::vector<UCConfigEntry>
sUserConfig;

static const IOHandle
sUCHandle = 0x12345678;

static bool
readSetting(UCSysConfig &setting)
{
   for (auto &entry : sUserConfig) {
      if (entry.name.compare(setting.name) != 0) {
         continue;
      }

      if (setting.data && setting.dataSize) {
         if (entry.type != setting.dataType) {
            gLog->error("Mismatch userconfig data type for {}, found {} expected {}", entry.name, setting.dataType, entry.type);
         }

         if (entry.data.size() != setting.dataSize) {
            gLog->error("Mismatch userconfig data size for {}, found {} expected {}", entry.name, setting.dataSize, entry.data.size());
            entry.data.resize(setting.dataSize);
         }

         std::memcpy(setting.data.get(), entry.data.data(), setting.dataSize);
      }

      return true;
   }

   return false;
}

static bool
updateSetting(UCSysConfig &setting)
{
   for (auto &entry : sUserConfig) {
      if (entry.name.compare(setting.name) != 0) {
         continue;
      }

      if (setting.data && setting.dataSize) {
         if (entry.type != setting.dataType) {
            gLog->error("Mismatch userconfig data type for {}, found {} expected {}", entry.name, setting.dataType, entry.type);
         }

         if (entry.data.size() != setting.dataSize) {
            gLog->error("Mismatch userconfig data size for {}, found {} expected {}", entry.name, setting.dataSize, entry.data.size());
            entry.data.resize(setting.dataSize);
         }

         std::memcpy(entry.data.data(), setting.data.get(), setting.dataSize);
      }

      return true;
   }

   return false;
}

IOHandle
UCOpen()
{
   return sUCHandle;
}

void
UCClose(IOHandle handle)
{
   decaf_check(handle == sUCHandle);
}

IOError
UCReadSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   if (handle != sUCHandle) {
      return IOError::Generic;
   }

   for (auto i = 0u; i < count; ++i) {
      auto &setting = settings[i];

      if (!readSetting(setting)) {
         gLog->warn("UCReadSysConfig unknown setting {} (type: {} size: {})", setting.name, setting.dataType, setting.dataSize);
      }
   }

   return IOError::OK;
}

IOError
UCWriteSysConfig(IOHandle handle, uint32_t count, UCSysConfig *settings)
{
   if (handle != sUCHandle) {
      return IOError::Generic;
   }

   for (auto i = 0u; i < count; ++i) {
      auto &setting = settings[i];

      if (!updateSetting(setting)) {
         gLog->debug("UCWriteSysConfig new setting {} (type: {} size: {})", setting.name, setting.dataType, setting.dataSize);

         // Create new entry
         auto entry = UCConfigEntry {};
         entry.name = setting.name;
         entry.type = setting.dataType;
         entry.data.resize(setting.dataSize);
         std::memcpy(entry.data.data(), setting.data.get(), setting.dataSize);
         sUserConfig.emplace_back(std::move(entry));
      }
   }

   return IOError::OK;
}

static void
addGroup(const std::string &name)
{
   auto entry = UCConfigEntry {};
   entry.name = name;
   entry.type = UCDataType::Group;
   sUserConfig.emplace_back(std::move(entry));
}

static void
addValue(const std::string &name, UCDataType type, uint32_t value)
{
   auto entry = UCConfigEntry {};
   entry.name = name;
   entry.type = type;

   switch (type) {
   case UCDataType::Uint8:
      entry.data.resize(1);
      *reinterpret_cast<be_val<uint8_t> *>(entry.data.data()) = value;
      break;
   case UCDataType::Uint16:
      entry.data.resize(2);
      *reinterpret_cast<be_val<uint16_t> *>(entry.data.data()) = value;
      break;
   case UCDataType::Uint32:
      entry.data.resize(4);
      *reinterpret_cast<be_val<uint32_t> *>(entry.data.data()) = value;
      break;
   default:
      decaf_abort(fmt::format("Invalid user config type {}", enumAsString(type)));
   }

   sUserConfig.emplace_back(std::move(entry));
}

static void
addBlob(const std::string &name, const std::vector<uint8_t> &data)
{
   auto entry = UCConfigEntry {};
   entry.name = name;
   entry.type = UCDataType::Blob;
   entry.data = data;
   sUserConfig.emplace_back(std::move(entry));
}

void
Module::initialiseUserConfig()
{
   auto invisible_titles = std::vector<uint8_t> {};
   invisible_titles.resize(512, 0);

   sUserConfig.clear();

   addGroup("cafe");
   addValue("cafe.cntry_reg",             UCDataType::Uint32,  SCICountryCode::USA);
   addValue("cafe.eco",                   UCDataType::Uint8,   0);
   addValue("cafe.eula_agree",            UCDataType::Uint8,   1);
   addValue("cafe.eula_version",          UCDataType::Uint32,  0);
   addValue("cafe.fast_boot",             UCDataType::Uint8,   0);
   addValue("cafe.initial_launch",        UCDataType::Uint8,   0);
   addValue("cafe.language",              UCDataType::Uint32,  SCILanguage::English);
   addValue("cafe.version",               UCDataType::Uint16,  0);

   addGroup("caffeine");
   addValue("caffeine.version",           UCDataType::Uint16,  0);
   addValue("caffeine.enable",            UCDataType::Uint8,   0);
   addValue("caffeine.ad_enable",         UCDataType::Uint8,   0);
   addValue("caffeine.push_enable",       UCDataType::Uint8,   1);
   addValue("caffeine.push_time_slot",    UCDataType::Uint32,  0);
   addValue("caffeine.push_interval",     UCDataType::Uint16,  0);
   addValue("caffeine.drcled_enable",     UCDataType::Uint8,   0);
   addValue("caffeine.push_capabilty",    UCDataType::Uint16,  0);
   addBlob ("caffeine.invisible_titles",  invisible_titles);

   addGroup("parent");
   addValue("parent.enable",              UCDataType::Uint8,   0);
}

void
Module::registerUserConfigFunctions()
{
   RegisterKernelFunction(UCOpen);
   RegisterKernelFunction(UCClose);
   RegisterKernelFunction(UCReadSysConfig);
   RegisterKernelFunction(UCWriteSysConfig);
}

} // namespace coreinit
