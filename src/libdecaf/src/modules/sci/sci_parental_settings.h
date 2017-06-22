#pragma once
#include "sci_enum.h"

#include <array>
#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

namespace sci
{

struct SCIParentalSettings
{
   be_val<uint16_t> version;
   be_val<uint8_t> enable;
   std::array<char, 5> pin_code;
   be_val<uint8_t> sec_question;
   std::array<char, 0x101> sec_answer;
   std::array<char, 0xCD> custom_sec_question;
   std::array<char, 0x401> email_address;
   be_val<uint8_t> permit_delete_all;
   be_val<uint32_t> rating_organization;
};
CHECK_OFFSET(SCIParentalSettings, 0x00, version);
CHECK_OFFSET(SCIParentalSettings, 0x02, enable);
CHECK_OFFSET(SCIParentalSettings, 0x03, pin_code);
CHECK_OFFSET(SCIParentalSettings, 0x08, sec_question);
CHECK_OFFSET(SCIParentalSettings, 0x09, sec_answer);
CHECK_OFFSET(SCIParentalSettings, 0x10A, custom_sec_question);
CHECK_OFFSET(SCIParentalSettings, 0x1D7, email_address);
CHECK_OFFSET(SCIParentalSettings, 0x5D8, permit_delete_all);
CHECK_OFFSET(SCIParentalSettings, 0x5DC, rating_organization);
CHECK_SIZE(SCIParentalSettings, 0x5E0);

SCIError
SCIInitParentalSettings(SCIParentalSettings *data);

} // namespace sci
