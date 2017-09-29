#include "sci_parental_settings.h"
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
sciCheckSecurityQuestion(virt_ptr<uint8_t> value)
{
   if (*value > 5u && *value != 0xFFu) {
      *value = uint8_t { 5 };
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckRatingOrganization(virt_ptr<uint32_t> value)
{
   if (*value > 0x10u) {
      *value = uint8_t { 0xFF };
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckParentalSettings(SCIParentalSettings *data)
{
   auto result = SCIError::OK;

   if (sciCheckSecurityQuestion(virt_addrof(data->sec_question)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (sciCheckRatingOrganization(virt_addrof(data->rating_organization)) != SCIError::OK) {
      result = SCIError::Error;
   }

   if (data->permit_delete_all > 1) {
      data->permit_delete_all = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->enable > 1) {
      data->enable = uint8_t { 1 };
      result = SCIError::Error;
   }

   return result;
}

} // namespace internal

SCIError
SCIInitParentalSettings(SCIParentalSettings *data)
{
   auto result = coreinit::UCOpen();
   if (result < 0) {
      return SCIError::Error;
   }

   auto handle = static_cast<coreinit::IOSHandle>(result);
   std::memset(data, 0, sizeof(SCIParentalSettings));

   UCSysConfig settings[] = {
      { "parent",                      0x777, UCDataType::Complex,         UCError::OK, 0, nullptr },
      { "parent.version",              0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->version),             virt_addrof(data->version) },
      { "parent.enable",               0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->enable),              virt_addrof(data->enable) },
      { "parent.pin_code",             0x777, UCDataType::String,          UCError::OK, sizeof(data->pin_code),            virt_addrof(data->pin_code) },
      { "parent.sec_question",         0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->sec_question),        virt_addrof(data->sec_question) },
      { "parent.sec_answer",           0x777, UCDataType::String,          UCError::OK, sizeof(data->sec_answer),          virt_addrof(data->sec_answer) },
      { "parent.custom_sec_question",  0x777, UCDataType::String,          UCError::OK, sizeof(data->custom_sec_question), virt_addrof(data->custom_sec_question) },
      { "parent.email_address",        0x777, UCDataType::String,          UCError::OK, sizeof(data->email_address),       virt_addrof(data->email_address) },
      { "parent.permit_delete_all",    0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->permit_delete_all),   virt_addrof(data->permit_delete_all) },
      { "parent.rating_organization",  0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->rating_organization), virt_addrof(data->rating_organization) },
   };

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = uint16_t { 10 };
         data->enable = uint8_t { 0 };
         data->pin_code.fill(0);
         data->sec_question = uint8_t { 0 };
         data->sec_answer.fill(0);
         data->custom_sec_question.fill(0);
         data->email_address.fill(0);
         data->permit_delete_all = uint8_t { 0 };
         data->rating_organization = uint8_t { 0 };
      }

      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   if (!internal::sciCheckParentalSettings(data)) {
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
