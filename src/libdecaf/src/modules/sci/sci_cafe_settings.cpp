#include "sci_cafe_settings.h"
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
sciCheckCafeSettingsLanguage(be_val<SCILanguage> *language)
{
   if (*language >= SCILanguage::Max) {
      *language = SCILanguage::Invalid;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckCafeSettingsCountry(be_val<SCICountry> *country)
{
   if (*country > SCICountry::Max) {
      *country = SCICountry::Max;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckCafeSettings(SCICafeSettings *data)
{
   auto result = SCIError::OK;

   if (sciCheckCafeSettingsLanguage(&data->language) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (sciCheckCafeSettingsCountry(&data->cntry_reg) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (data->eula_agree > 1) {
      data->eula_agree = 1;
      result = SCIError::Error;
   }

   if (data->eco > 1) {
      data->eco = 1;
      result = SCIError::Error;
   }

   if (data->fast_boot > 1) {
      data->fast_boot = 1;
      result = SCIError::Error;
   }

   return result;
}

} // namespace internal

SCIError
SCIInitCafeSettings(SCICafeSettings *data)
{
   auto result = coreinit::UCOpen();
   if (result < 0) {
      return SCIError::Error;
   }

   auto handle = static_cast<coreinit::IOSHandle>(result);
   std::memset(data, 0, sizeof(SCICafeSettings));

   UCSysConfig settings[] = {
      { "cafe",                  0x777, UCDataType::Complex,         UCError::OK, 0, nullptr },
      { "cafe.version",          0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->version), &data->version },
      { "cafe.language",         0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->language), &data->language },
      { "cafe.cntry_reg",        0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->cntry_reg), &data->cntry_reg },
      { "cafe.eula_agree",       0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->eula_agree), &data->eula_agree },
      { "cafe.eula_version",     0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->eula_version), &data->eula_version },
      { "cafe.initial_launch",   0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->initial_launch), &data->initial_launch },
      { "cafe.eco",              0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->eco), &data->eco },
      { "cafe.fast_boot",        0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->fast_boot), &data->fast_boot },
   };

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = 5;
         data->language = SCILanguage::Japanese;
         data->cntry_reg = SCICountry::Japan;
         data->eula_agree = 0;
         data->eula_version = 0;
         data->initial_launch = 0;
         data->eco = 0;
         data->fast_boot = 0;
      }

      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   if (!internal::sciCheckCafeSettings(data)) {
      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   coreinit::UCClose(handle);
   return SCIError::OK;
}

} // namespace sci
