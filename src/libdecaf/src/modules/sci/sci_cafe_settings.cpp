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
sciCheckCafeSettingsLanguage(virt_ptr<SCILanguage> language)
{
   if (*language >= SCILanguage::Max) {
      *language = SCILanguage::Invalid;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckCafeSettingsCountry(virt_ptr<SCICountry> country)
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

   if (sciCheckCafeSettingsLanguage(virt_addrof(data->language)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (sciCheckCafeSettingsCountry(virt_addrof(data->cntry_reg)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (data->eula_agree > 1u) {
      data->eula_agree = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->eco > 1u) {
      data->eco = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->fast_boot > 1u) {
      data->fast_boot = uint8_t { 1 };
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
      { "cafe.version",          0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->version),          virt_addrof(data->version) },
      { "cafe.language",         0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->language),         virt_addrof(data->language) },
      { "cafe.cntry_reg",        0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->cntry_reg),        virt_addrof(data->cntry_reg) },
      { "cafe.eula_agree",       0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->eula_agree),       virt_addrof(data->eula_agree) },
      { "cafe.eula_version",     0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->eula_version),     virt_addrof(data->eula_version) },
      { "cafe.initial_launch",   0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->initial_launch),   virt_addrof(data->initial_launch) },
      { "cafe.eco",              0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->eco),              virt_addrof(data->eco) },
      { "cafe.fast_boot",        0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->fast_boot),        virt_addrof(data->fast_boot) },
   };

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = uint16_t { 5 };
         data->language = SCILanguage::Japanese;
         data->cntry_reg = SCICountry::Japan;
         data->eula_agree = uint8_t { 0 };
         data->eula_version = uint32_t { 0 };
         data->initial_launch = uint8_t { 0 };
         data->eco = uint8_t { 0 };
         data->fast_boot = uint8_t { 0 };
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
