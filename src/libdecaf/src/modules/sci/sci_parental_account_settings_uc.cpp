#include "sci_parental_account_settings_uc.h"
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
sciCheckPermission(virt_ptr<uint8_t> value)
{
   if (*value > 2u) {
      *value = uint8_t { 2 };
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckParentalAccountSettingsUC(SCIParentalAccountSettingsUC *data)
{
   auto result = SCIError::OK;

   if (data->eshop_purchase > 1u) {
      data->eshop_purchase = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->friend_reg > 1u) {
      data->friend_reg = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->acct_modify > 1u) {
      data->acct_modify = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->data_manage > 1u) {
      data->data_manage = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->int_setting > 1u) {
      data->int_setting = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->country_setting > 1u) {
      data->country_setting = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->sys_init > 1u) {
      data->sys_init = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (data->int_browser > 1u) {
      data->int_browser = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (!sciCheckPermission(virt_addrof(data->int_movie))) {
      result = SCIError::Error;
   }

   if (data->net_communication_on_game > 1u) {
      data->net_communication_on_game = uint8_t { 1 };
      result = SCIError::Error;
   }

   if (!sciCheckPermission(virt_addrof(data->network_launcher))) {
      result = SCIError::Error;
   }

   if (data->entertainment_launcher > 1u) {
      data->entertainment_launcher = uint8_t { 1 };
      result = SCIError::Error;
   }

   return result;
}

} // namespace internal

SCIError
SCIInitParentalAccountSettingsUC(SCIParentalAccountSettingsUC *data,
                                 uint32_t account)
{
   auto result = coreinit::UCOpen();
   if (result < 0) {
      return SCIError::Error;
   }

   auto handle = static_cast<coreinit::IOSHandle>(result);
   std::memset(data, 0, sizeof(SCIParentalAccountSettingsUC));

   UCSysConfig settings[] = {
      { {}, 0x777, UCDataType::Complex,         UCError::OK, 0, nullptr },
      { {}, 0x777, UCDataType::UnsignedShort,   UCError::OK, sizeof(data->version),                   virt_addrof(data->version) },
      { {}, 0x777, UCDataType::UnsignedInt,     UCError::OK, sizeof(data->game_rating),               virt_addrof(data->game_rating) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->eshop_purchase),            virt_addrof(data->eshop_purchase) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->friend_reg),                virt_addrof(data->friend_reg) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->acct_modify),               virt_addrof(data->acct_modify) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->data_manage),               virt_addrof(data->data_manage) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->int_setting),               virt_addrof(data->int_setting) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->country_setting),           virt_addrof(data->country_setting) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->sys_init),                  virt_addrof(data->sys_init) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->int_browser),               virt_addrof(data->int_browser) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->int_movie),                 virt_addrof(data->int_movie) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->net_communication_on_game), virt_addrof(data->net_communication_on_game) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->network_launcher),          virt_addrof(data->network_launcher) },
      { {}, 0x777, UCDataType::UnsignedByte,    UCError::OK, sizeof(data->entertainment_launcher),    virt_addrof(data->entertainment_launcher) },
   };

   snprintf(virt_addrof(settings[0].name).getRawPointer(), settings[0].name.size(), "p_acct%d", account);
   snprintf(virt_addrof(settings[1].name).getRawPointer(), settings[1].name.size(), "p_acct%d.version", account);
   snprintf(virt_addrof(settings[2].name).getRawPointer(), settings[2].name.size(), "p_acct%d.game_rating", account);
   snprintf(virt_addrof(settings[3].name).getRawPointer(), settings[3].name.size(), "p_acct%d.eshop_purchase", account);
   snprintf(virt_addrof(settings[4].name).getRawPointer(), settings[4].name.size(), "p_acct%d.friend_reg", account);
   snprintf(virt_addrof(settings[5].name).getRawPointer(), settings[5].name.size(), "p_acct%d.acct_modify", account);
   snprintf(virt_addrof(settings[6].name).getRawPointer(), settings[6].name.size(), "p_acct%d.data_manage", account);
   snprintf(virt_addrof(settings[7].name).getRawPointer(), settings[7].name.size(), "p_acct%d.int_setting", account);
   snprintf(virt_addrof(settings[8].name).getRawPointer(), settings[8].name.size(), "p_acct%d.country_setting", account);
   snprintf(virt_addrof(settings[9].name).getRawPointer(), settings[9].name.size(), "p_acct%d.sys_init", account);
   snprintf(virt_addrof(settings[10].name).getRawPointer(), settings[10].name.size(), "p_acct%d.int_browser", account);
   snprintf(virt_addrof(settings[11].name).getRawPointer(), settings[11].name.size(), "p_acct%d.int_movie", account);
   snprintf(virt_addrof(settings[12].name).getRawPointer(), settings[12].name.size(), "p_acct%d.net_communication_on_game", account);
   snprintf(virt_addrof(settings[13].name).getRawPointer(), settings[13].name.size(), "p_acct%d.network_launcher", account);
   snprintf(virt_addrof(settings[14].name).getRawPointer(), settings[14].name.size(), "p_acct%d.entertainment_launcher", account);

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::KeyNotFound) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = uint16_t { 10 };
         data->game_rating = uint32_t { 18 };
         data->eshop_purchase = uint8_t { 0 };
         data->friend_reg = uint8_t { 0 };
         data->acct_modify = uint8_t { 0 };
         data->data_manage = uint8_t { 0 };
         data->int_setting = uint8_t { 0 };
         data->country_setting = uint8_t { 0 };
         data->sys_init = uint8_t { 0 };
         data->int_browser = uint8_t { 0 };
         data->int_movie = uint8_t { 0 };
         data->net_communication_on_game = uint8_t { 0 };
         data->network_launcher = uint8_t { 0 };
         data->entertainment_launcher = uint8_t { 0 };
      }

      result = coreinit::UCWriteSysConfig(handle, COUNT_OF(settings), settings);
      if (result != UCError::OK) {
         coreinit::UCClose(handle);
         return SCIError::WriteError;
      }
   }

   if (!internal::sciCheckParentalAccountSettingsUC(data)) {
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
