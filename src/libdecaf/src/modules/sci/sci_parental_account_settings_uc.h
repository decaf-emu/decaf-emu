#pragma once
#include "sci_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace sci
{

struct SCIParentalAccountSettingsUC
{
   be2_val<uint16_t> version;
   PADDING(2);
   be2_val<uint32_t> game_rating;
   be2_val<uint8_t> eshop_purchase;
   be2_val<uint8_t> friend_reg;
   be2_val<uint8_t> acct_modify;
   be2_val<uint8_t> data_manage;
   be2_val<uint8_t> int_setting;
   be2_val<uint8_t> country_setting;
   be2_val<uint8_t> sys_init;
   be2_val<uint8_t> int_browser;
   be2_val<uint8_t> int_movie;
   be2_val<uint8_t> net_communication_on_game;
   be2_val<uint8_t> network_launcher;
   be2_val<uint8_t> entertainment_launcher;
};
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x00, version);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x04, game_rating);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x08, eshop_purchase);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x09, friend_reg);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0A, acct_modify);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0B, data_manage);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0C, int_setting);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0D, country_setting);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0E, sys_init);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x0F, int_browser);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x10, int_movie);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x11, net_communication_on_game);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x12, network_launcher);
CHECK_OFFSET(SCIParentalAccountSettingsUC, 0x13, entertainment_launcher);
CHECK_SIZE(SCIParentalAccountSettingsUC, 0x14);

SCIError
SCIInitParentalAccountSettingsUC(SCIParentalAccountSettingsUC *data,
                                 uint32_t account);

} // namespace sci
