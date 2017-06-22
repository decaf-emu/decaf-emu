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
sciCheckPushInterval(be_val<uint16_t> *value)
{
   if (*value > 1440) {
      *value = 1440;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckPushTimeSlot(be_val<uint32_t> *value)
{
   if (*value & 0x80000000) {
      *value &= 0x7FFFFFFF;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckCaffeineSettings(SCICaffeineSettings *data)
{
   auto result = SCIError::OK;

   if (sciCheckPushTimeSlot(&data->push_time_slot) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (sciCheckPushInterval(&data->push_interval) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (data->enable > 1) {
      data->enable = 1;
      result = SCIError::Error;
   }

   if (data->ad_enable > 1) {
      data->ad_enable = 1;
      result = SCIError::Error;
   }

   if (data->push_enable > 1) {
      data->push_enable = 1;
      result = SCIError::Error;
   }

   if (data->drcled_enable > 1) {
      data->drcled_enable = 1;
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
      { "caffeine",                    0x777, UCDataType::Group,  UCError::OK, 0, nullptr },
      { "caffeine.version",            0x777, UCDataType::Uint16, UCError::OK, sizeof(data->version), &data->version },
      { "caffeine.enable",             0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->enable), &data->enable },
      { "caffeine.ad_enable",          0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->ad_enable), &data->ad_enable },
      { "caffeine.push_enable",        0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->push_enable), &data->push_enable },
      { "caffeine.push_time_slot",     0x777, UCDataType::Uint32, UCError::OK, sizeof(data->push_time_slot), &data->push_time_slot },
      { "caffeine.push_interval",      0x777, UCDataType::Uint16, UCError::OK, sizeof(data->push_interval), &data->push_interval },
      { "caffeine.drcled_enable",      0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->drcled_enable), &data->drcled_enable },
      { "caffeine.push_capabilty",     0x777, UCDataType::Uint16, UCError::OK, sizeof(data->push_capabilty), &data->push_capabilty },
      { "caffeine.invisible_titles",   0x777, UCDataType::Uint32, UCError::OK, sizeof(data->invisible_titles), &data->invisible_titles },
   };

   result = coreinit::UCReadSysConfig(handle, 10, settings);
   if (result != UCError::OK) {
      if (result != UCError::UnkError9) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = 3;
         data->enable = 1;
         data->ad_enable = 1;
         data->push_enable = 0;
         data->push_time_slot = 0x7F1FFE00;
         data->push_interval = 0x1A4;
         data->drcled_enable = 1;
         data->push_capabilty = 0xFFFF;
         data->invisible_titles.fill(0);
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
