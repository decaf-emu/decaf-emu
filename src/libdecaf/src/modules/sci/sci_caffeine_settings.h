#pragma once
#include "sci_enum.h"

#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace sci
{

struct SCICaffeineSettings
{
   be2_val<uint16_t> version;
   be2_val<uint8_t> enable;
   be2_val<uint8_t> ad_enable;
   be2_val<uint8_t> push_enable;
   PADDING(3);
   be2_val<uint32_t> push_time_slot;
   be2_val<uint16_t> push_interval;
   be2_val<uint8_t> drcled_enable;
   PADDING(1);
   be2_val<uint16_t> push_capabilty;
   PADDING(6);
   be2_array<uint8_t, 0x200> invisible_titles;
};
CHECK_OFFSET(SCICaffeineSettings, 0x00, version);
CHECK_OFFSET(SCICaffeineSettings, 0x02, enable);
CHECK_OFFSET(SCICaffeineSettings, 0x03, ad_enable);
CHECK_OFFSET(SCICaffeineSettings, 0x04, push_enable);
CHECK_OFFSET(SCICaffeineSettings, 0x08, push_time_slot);
CHECK_OFFSET(SCICaffeineSettings, 0x0C, push_interval);
CHECK_OFFSET(SCICaffeineSettings, 0x0E, drcled_enable);
CHECK_OFFSET(SCICaffeineSettings, 0x10, push_capabilty);
CHECK_OFFSET(SCICaffeineSettings, 0x18, invisible_titles);
CHECK_SIZE(SCICaffeineSettings, 0x218);

SCIError
SCIInitCaffeineSettings(SCICaffeineSettings *data);

} // namespace sci
