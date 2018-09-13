#pragma once
#include "ios_mcp_enum.h"
#include "ios/auxil/ios_auxil_config.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::mcp::internal
{

#pragma pack(push, 1)

struct RtcConfig
{
   be2_val<uint32_t> version;
   be2_val<uint32_t> rtc_offset;
};
CHECK_OFFSET(RtcConfig, 0x00, version);
CHECK_OFFSET(RtcConfig, 0x04, rtc_offset);
CHECK_SIZE(RtcConfig, 0x08);

struct SystemConfig
{
   be2_val<uint32_t> version;
   be2_val<uint32_t> cmdFlags;
   be2_val<uint64_t> default_os_id;
   be2_val<uint64_t> default_title_id;

   struct
   {
      be2_val<uint32_t> enable;
      be2_val<uint32_t> max_size;
   } log;

   struct
   {
      be2_val<uint32_t> enable;
   } standby;

   struct
   {
      be2_val<uint32_t> cache_user_code;
      be2_val<uint32_t> max_file_size;
      be2_val<uint32_t> cache_delay_ms;
   } ramdisk;

   be2_val<uint32_t> simulated_ppc_mem2_size;
   be2_val<uint32_t> dev_mode;
   be2_val<uint64_t> prev_title_id;
   be2_val<uint64_t> prev_os_id;
   be2_val<uint32_t> default_app_type;
   be2_array<char, 16> default_device_type;
   be2_val<uint32_t> default_device_index;
   be2_val<uint32_t> fast_relaunch_value;
   be2_val<uint64_t> default_eco_title_id;
};
CHECK_OFFSET(SystemConfig, 0x00, version);
CHECK_OFFSET(SystemConfig, 0x04, cmdFlags);
CHECK_OFFSET(SystemConfig, 0x08, default_os_id);
CHECK_OFFSET(SystemConfig, 0x10, default_title_id);
CHECK_OFFSET(SystemConfig, 0x18, log.enable);
CHECK_OFFSET(SystemConfig, 0x1C, log.max_size);
CHECK_OFFSET(SystemConfig, 0x20, standby.enable);
CHECK_OFFSET(SystemConfig, 0x24, ramdisk.cache_user_code);
CHECK_OFFSET(SystemConfig, 0x28, ramdisk.max_file_size);
CHECK_OFFSET(SystemConfig, 0x2C, ramdisk.cache_delay_ms);
CHECK_OFFSET(SystemConfig, 0x30, simulated_ppc_mem2_size);
CHECK_OFFSET(SystemConfig, 0x34, dev_mode);
CHECK_OFFSET(SystemConfig, 0x38, prev_title_id);
CHECK_OFFSET(SystemConfig, 0x40, prev_os_id);
CHECK_OFFSET(SystemConfig, 0x48, default_app_type);
CHECK_OFFSET(SystemConfig, 0x4C, default_device_type);
CHECK_OFFSET(SystemConfig, 0x5C, default_device_index);
CHECK_OFFSET(SystemConfig, 0x60, fast_relaunch_value);
CHECK_OFFSET(SystemConfig, 0x64, default_eco_title_id);
CHECK_SIZE(SystemConfig, 0x6C);

struct SysProdConfig
{
   be2_val<MCPRegion> product_area;
   be2_val<uint16_t> eeprom_version;
   PADDING(2);
   be2_val<MCPRegion> game_region;
   UNKNOWN(4);
   be2_array<char, 5> ntsc_pal;

   //! Actually 5ghz_country_code, but can't start a variable with a number!!
   be2_array<char, 4> wifi_5ghz_country_code;

   //! Actually 5ghz_country_code_revision, but can't start a variable with a number!!
   be2_val<uint8_t> wifi_5ghz_country_code_revision;

   be2_array<char, 8> code_id;
   be2_array<char, 12> serial_id;
   UNKNOWN(4);
   be2_array<char, 16> model_number;
   be2_val<uint32_t> version;
};
CHECK_OFFSET(SysProdConfig, 0x00, product_area);
CHECK_OFFSET(SysProdConfig, 0x04, eeprom_version);
CHECK_OFFSET(SysProdConfig, 0x08, game_region);
CHECK_OFFSET(SysProdConfig, 0x10, ntsc_pal);
CHECK_OFFSET(SysProdConfig, 0x15, wifi_5ghz_country_code);
CHECK_OFFSET(SysProdConfig, 0x19, wifi_5ghz_country_code_revision);
CHECK_OFFSET(SysProdConfig, 0x1A, code_id);
CHECK_OFFSET(SysProdConfig, 0x22, serial_id);
CHECK_OFFSET(SysProdConfig, 0x32, model_number);
CHECK_OFFSET(SysProdConfig, 0x42, version);
CHECK_SIZE(SysProdConfig, 0x46);

#pragma pack(pop)

MCPError
translateUCError(auxil::UCError error);

MCPError
readConfigItems(phys_ptr<auxil::UCItem> items,
                uint32_t count);

MCPError
writeConfigItems(phys_ptr<auxil::UCItem> items,
                 uint32_t count);

MCPError
deleteConfigItems(phys_ptr<auxil::UCItem> items,
                  uint32_t count);

MCPError
loadRtcConfig();

MCPError
loadSystemConfig();

MCPError
loadSysProdConfig();

phys_ptr<RtcConfig>
getRtcConfig();

phys_ptr<SystemConfig>
getSystemConfig();

phys_ptr<SysProdConfig>
getSysProdConfig();

void
initialiseStaticConfigData();

} // namespace ios::mcp::internal
