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
sciCheckPermission(be_val<uint8_t> *value)
{
   if (*value > 2) {
      *value = 2;
      return SCIError::Error;
   }

   return SCIError::OK;
}

static SCIError
sciCheckParentalAccountSettingsUC(SCIParentalAccountSettingsUC *data)
{
   auto result = SCIError::OK;

   if (data->eshop_purchase > 1) {
      data->eshop_purchase = 1;
      result = SCIError::Error;
   }

   if (data->friend_reg > 1) {
      data->friend_reg = 1;
      result = SCIError::Error;
   }

   if (data->acct_modify > 1) {
      data->acct_modify = 1;
      result = SCIError::Error;
   }

   if (data->data_manage > 1) {
      data->data_manage = 1;
      result = SCIError::Error;
   }

   if (data->int_setting > 1) {
      data->int_setting = 1;
      result = SCIError::Error;
   }

   if (data->country_setting > 1) {
      data->country_setting = 1;
      result = SCIError::Error;
   }

   if (data->sys_init > 1) {
      data->sys_init = 1;
      result = SCIError::Error;
   }

   if (data->int_browser > 1) {
      data->int_browser = 1;
      result = SCIError::Error;
   }

   if (!sciCheckPermission(&data->int_movie)) {
      result = SCIError::Error;
   }

   if (data->net_communication_on_game > 1) {
      data->net_communication_on_game = 1;
      result = SCIError::Error;
   }

   if (!sciCheckPermission(&data->network_launcher)) {
      result = SCIError::Error;
   }

   if (data->entertainment_launcher > 1) {
      data->entertainment_launcher = 1;
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
      { {}, 0x777, UCDataType::Group,  UCError::OK, 0, nullptr },
      { {}, 0x777, UCDataType::Uint16, UCError::OK, sizeof(data->version), &data->version },
      { {}, 0x777, UCDataType::Uint32, UCError::OK, sizeof(data->game_rating), &data->game_rating },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->eshop_purchase), &data->eshop_purchase },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->friend_reg), &data->friend_reg },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->acct_modify), &data->acct_modify },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->data_manage), &data->data_manage },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->int_setting), &data->int_setting },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->country_setting), &data->country_setting },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->sys_init), &data->sys_init },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->int_browser), &data->int_browser },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->int_movie), &data->int_movie },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->net_communication_on_game), &data->net_communication_on_game },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->network_launcher), &data->network_launcher },
      { {}, 0x777, UCDataType::Uint8,  UCError::OK, sizeof(data->entertainment_launcher), &data->entertainment_launcher },
   };

   snprintf(settings[0].name, sizeof(settings[0].name), "p_acct%d", account);
   snprintf(settings[1].name, sizeof(settings[1].name), "p_acct%d.version", account);
   snprintf(settings[2].name, sizeof(settings[2].name), "p_acct%d.game_rating", account);
   snprintf(settings[3].name, sizeof(settings[3].name), "p_acct%d.eshop_purchase", account);
   snprintf(settings[4].name, sizeof(settings[4].name), "p_acct%d.friend_reg", account);
   snprintf(settings[5].name, sizeof(settings[5].name), "p_acct%d.acct_modify", account);
   snprintf(settings[6].name, sizeof(settings[6].name), "p_acct%d.data_manage", account);
   snprintf(settings[7].name, sizeof(settings[7].name), "p_acct%d.int_setting", account);
   snprintf(settings[8].name, sizeof(settings[8].name), "p_acct%d.country_setting", account);
   snprintf(settings[9].name, sizeof(settings[9].name), "p_acct%d.sys_init", account);
   snprintf(settings[10].name, sizeof(settings[10].name), "p_acct%d.int_browser", account);
   snprintf(settings[11].name, sizeof(settings[11].name), "p_acct%d.int_movie", account);
   snprintf(settings[12].name, sizeof(settings[12].name), "p_acct%d.net_communication_on_game", account);
   snprintf(settings[13].name, sizeof(settings[13].name), "p_acct%d.network_launcher", account);
   snprintf(settings[14].name, sizeof(settings[14].name), "p_acct%d.entertainment_launcher", account);

   result = coreinit::UCReadSysConfig(handle, COUNT_OF(settings), settings);
   if (result != UCError::OK) {
      if (result != UCError::UnkError9) {
         coreinit::UCDeleteSysConfig(handle, 1, settings);

         data->version = 10;
         data->game_rating = 18;
         data->eshop_purchase = 0;
         data->friend_reg = 0;
         data->acct_modify = 0;
         data->data_manage = 0;
         data->int_setting = 0;
         data->country_setting = 0;
         data->sys_init = 0;
         data->int_browser = 0;
         data->int_movie = 0;
         data->net_communication_on_game = 0;
         data->network_launcher = 0;
         data->entertainment_launcher = 0;
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
