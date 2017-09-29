#include "sci_caffeine_settings.h"
#include "modules/coreinit/coreinit_userconfig.h"

#include <common/count_of.h>

using coreinit::UCDataType;
using coreinit::UCError;
using coreinit::UCSysConfig;

namespace sci
{

namespace internal
{

static SCIError
sciCheckPushInterval(virt_ptr<uint16_t> value)
{
   if (*value > 1440u) {
      *value = uint16_t { 1440 };
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckPushTimeSlot(virt_ptr<uint32_t> value)
{
   if (*value & 0x80000000u) {
      *value &= 0x7FFFFFFFu;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckCaffeineSettings(SCICaffeineSettings *data)
{
   auto result = SCIError::OK;

   if (sciCheckPushTimeSlot(virt_addrof(data->push_time_slot)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (sciCheckPushInterval(virt_addrof(data->push_interval)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (data->enable > 1u) {
      data->enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->ad_enable > 1u) {
      data->ad_enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->push_enable > 1u) {
      data->push_enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->drcled_enable > 1u) {
      data->drcled_enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   return result;
}

} // namespace internal

SCIError
SCIInitCaffeineSettings(SCICaffeineSettings *data)
{
   auto result = coreinit::UCOpen();
   if (result < 0) {
      return SCIError::Error;
   }

   auto handle = static_cast<coreinit::IOSHandle>(result);
   std::memset(data, 0, sizeof(SCICaffeineSettings));

   UCSysConfig settings[] = {
      { "caffeine",                    0x777, UCDataType::Complex,         UCError::OK, 0, nullptr },
      { "caffeine.version",            0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->version),          virt_addrof(data->version) },
      { "caffeine.enable",             0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->enable),           virt_addrof(data->enable) },
      { "caffeine.ad_enable",          0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->ad_enable),        virt_addrof(data->ad_enable) },
      { "caffeine.push_enable",        0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->push_enable),      virt_addrof(data->push_enable) },
      { "caffeine.push_time_slot",     0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->push_time_slot),   virt_addrof(data->push_time_slot) },
      { "caffeine.push_interval",      0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->push_interval),    virt_addrof(data->push_interval) },
      { "caffeine.drcled_enable",      0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->drcled_enable),    virt_addrof(data->drcled_enable) },
      { "caffeine.push_capabilty",     0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->push_capabilty),   virt_addrof(data->push_capabilty) },
      { "caffeine.invisible_titles",   0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->invisible_titles), virt_addrof(data->invisible_titles) },
   };

   result = coreinit::UCReadSysConfig(handle, 10, settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = uint16_t { 3 };
         data->enable = uint8_t { 1 };
         data->ad_enable = uint8_t { 1 };
         data->push_enable = uint8_t { 0 };
         data->push_time_slot = uint32_t { 0x7F1FFE00u };
         data->push_interval = uint16_t { 0x1A4 };
         data->drcled_enable = uint8_t { 1 };
         data->push_capabilty = uint16_t { 0xFFFFu };
         data->invisible_titles.fill(0u);
      }

      result = coreinit::UCWriteSysConfig(handle, 10, settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   if (!internal::sciCheckCaffeineSettings(data)) {
      result = coreinit::UCWriteSysConfig(handle, 10, settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   coreinit::UCClose(handle);
   return SCIError::OK;
}

} // namespace sci
