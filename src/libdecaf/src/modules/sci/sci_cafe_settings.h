#pragma once
#include "sci_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace sci
{

struct SCICafeSettings
{
   be2_val<uint16_t> version;
   PADDING(2);
   be2_val<SCILanguage> language;
   be2_val<SCICountry> cntry_reg;
   be2_val<uint8_t> eula_agree;
   PADDING(3);
   be2_val<uint32_t> eula_version;
   be2_val<uint8_t> initial_launch;
   be2_val<uint8_t> eco;
   be2_val<uint8_t> fast_boot;
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
