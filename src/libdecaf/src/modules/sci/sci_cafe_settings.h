#pragma once
#include "sci_enum.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/structsize.h>

namespace sci
{

struct SCICafeSettings
{
   be_val<uint16_t> version;
   PADDING(2);
   be_val<SCILanguage> language;
   be_val<SCICountry> cntry_reg;
   be_val<uint8_t> eula_agree;
   PADDING(3);
   be_val<uint32_t> eula_version;
   be_val<uint8_t> initial_launch;
   be_val<uint8_t> eco;
   be_val<uint8_t> fast_boot;
   PADDING(1);
};
CHECK_OFFSET(SCICafeSettings, 0x00, version);
CHECK_OFFSET(SCICafeSettings, 0x04, language);
CHECK_OFFSET(SCICafeSettings, 0x08, cntry_reg);
CHECK_OFFSET(SCICafeSettings, 0x0C, eula_agree);
CHECK_OFFSET(SCICafeSettings, 0x10, eula_version);
CHECK_OFFSET(SCICafeSettings, 0x14, initial_launch);
CHECK_OFFSET(SCICafeSettings, 0x15, eco);
CHECK_OFFSET(SCICafeSettings, 0x16, fast_boot);
CHECK_SIZE(SCICafeSettings, 0x18);

SCIError
SCIInitCafeSettings(SCICafeSettings *data);

} // namespace sci
