#include "ios_mcp_config.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/ios_stackobject.h"

#include <array>
#include <libcpu/be2_struct.h>
#include <string_view>

using namespace ios::auxil;
using namespace ios::kernel;

namespace ios::mcp::internal
{

struct StaticConfigData
{
   be2_struct<RtcConfig> rtcConfig;
   be2_struct<SystemConfig> systemConfig;
   be2_struct<SysProdConfig> sysProdConfig;
};

static phys_ptr<StaticConfigData>
sData = nullptr;

static std::array<const char *, 5>
sValidRootKeys =
{
   "app",
   "eco",
   "rtc",
   "system",
   "sys_prod"
};

static bool
isValidRootKey(std::string_view key)
{
   for (auto validKey : sValidRootKeys) {
      if (key.compare(validKey) == 0) {
         return true;
      }
   }

   return false;
}

static std::string_view
getFileSysPath(UCFileSys fileSys)
{
   switch (fileSys) {
   case UCFileSys::Sys:
      return "/vol/system/config/";
   case UCFileSys::Slc:
      return "/vol/system_slc/config/";
   case UCFileSys::Ram:
      return "/vol/system_ram/config/";
   default:
      return "*error*";
   }
}

static MCPError
translateError(UCError error)
{
   switch (error) {
   case UCError::Other:
      return MCPError::Invalid;
   case UCError::System:
      return MCPError::System;
   case UCError::Alloc:
      return MCPError::Alloc;
   case UCError::Opcode:
      return MCPError::Opcode;
   case UCError::InvalidParam:
      return MCPError::InvalidParam;
   case UCError::InvalidType:
      return MCPError::InvalidType;
   case UCError::Unsupported:
      return MCPError::Unsupported;
   case UCError::NonLeafNode:
      return MCPError::NonLeafNode;
   case UCError::KeyNotFound:
      return MCPError::KeyNotFound;
   case UCError::Modify:
      return MCPError::Modify;
   case UCError::StringTooLong:
      return MCPError::StringTooLong;
   case UCError::RootKeysDiffer:
      return MCPError::RootKeysDiffer;
   case UCError::InvalidLocation:
      return MCPError::InvalidLocation;
   case UCError::BadComment:
      return MCPError::BadComment;
   case UCError::ReadAccess:
      return MCPError::ReadAccess;
   case UCError::WriteAccess:
      return MCPError::WriteAccess;
   case UCError::CreateAccess:
      return MCPError::CreateAccess;
   case UCError::FileSysName:
      return MCPError::FileSysName;
   case UCError::FileSysInit:
      return MCPError::FileSysInit;
   case UCError::FileSysMount:
      return MCPError::FileSysMount;
   case UCError::FileOpen:
      return MCPError::FileOpen;
   case UCError::FileStat:
      return MCPError::FileStat;
   case UCError::FileRead:
      return MCPError::FileRead;
   case UCError::FileWrite:
      return MCPError::FileWrite;
   case UCError::FileTooBig:
      return MCPError::FileTooBig;
   case UCError::FileRemove:
      return MCPError::FileRemove;
   case UCError::FileRename:
      return MCPError::FileRename;
   case UCError::FileClose:
      return MCPError::FileClose;
   case UCError::FileSeek:
      return MCPError::FileSeek;
   case UCError::MalformedXML:
      return MCPError::MalformedXML;
   case UCError::Version:
      return MCPError::Version;
   case UCError::NoIPCBuffers:
      return MCPError::NoIpcBuffers;
   case UCError::FileLockNeeded:
      return MCPError::FileLockNeeded;
   case UCError::SysProt:
      return MCPError::SysProt;
   default:
      return MCPError::Invalid;
   }
}

MCPError
readConfigItems(phys_ptr<UCItem> items,
                uint32_t count)
{
   if (count == 0) {
      return MCPError::OK;
   }

   auto name = std::string_view { phys_addrof(items[0].name).getRawPointer() };
   auto fileSys = getFileSys(name);
   if (fileSys == UCFileSys::Invalid) {
      return MCPError::InvalidLocation;
   }

   auto rootKey = getRootKey(name);
   if (!isValidRootKey(rootKey)) {
      return MCPError::FileSysName;
   }

   return translateError(readItems(getFileSysPath(fileSys), items, count, nullptr));
}

MCPError
writeConfigItems(phys_ptr<auxil::UCItem> items,
                 uint32_t count)
{
   if (count == 0) {
      return MCPError::OK;
   }

   auto name = std::string_view { phys_addrof(items[0].name).getRawPointer() };
   auto fileSys = getFileSys(name);
   if (fileSys == UCFileSys::Invalid) {
      return MCPError::InvalidLocation;
   }

   auto rootKey = getRootKey(name);
   if (!isValidRootKey(rootKey)) {
      return MCPError::FileSysName;
   }

   return translateError(writeItems(getFileSysPath(fileSys), items, count, nullptr));
}

MCPError
deleteConfigItems(phys_ptr<auxil::UCItem> items,
                  uint32_t count)
{
   if (count == 0) {
      return MCPError::OK;
   }

   auto name = std::string_view { phys_addrof(items[0].name).getRawPointer() };
   auto fileSys = getFileSys(name);
   if (fileSys == UCFileSys::Invalid) {
      return MCPError::InvalidLocation;
   }

   auto rootKey = getRootKey(name);
   if (!isValidRootKey(rootKey)) {
      return MCPError::FileSysName;
   }

   return translateError(deleteItems(getFileSysPath(fileSys), items, count));
}

MCPError
loadRtcConfig()
{
   StackArray<UCItem, 3> items;
   auto config = phys_addrof(sData->rtcConfig);

   items[0].name = "slc:rtc";
   items[0].access = 0x777u;
   items[0].dataType = UCDataType::Complex;

   items[1].name = "slc:rtc.version";
   items[1].dataType = UCDataType::UnsignedInt;
   items[1].dataSize = 4u;
   items[1].data = phys_addrof(config->version);

   items[2].name = "slc:rtc.rtc_offset";
   items[2].dataType = UCDataType::UnsignedInt;
   items[2].dataSize = 4u;
   items[2].data = phys_addrof(config->rtc_offset);

   auto error = readConfigItems(items, items.size());
   if (error < MCPError::OK || config->version < 21) {
      // Factory reset items
      std::memset(config.getRawPointer(), 0, sizeof(SysProdConfig));
      config->version = 1u;
      config->rtc_offset = 0x4EFFA200u;

      // Try save them to file
      deleteConfigItems(items, 1);
      writeConfigItems(items, items.size());
   }

   return MCPError::OK;
}

MCPError
loadSystemConfig()
{
   StackArray<UCItem, 20> items;
   auto config = phys_addrof(sData->systemConfig);

   items[0].name = "system";
   items[0].access = 0x777u;
   items[0].dataType = UCDataType::Complex;

   items[1].name = "system.version";
   items[1].dataType = UCDataType::UnsignedInt;
   items[1].dataSize = 4u;
   items[1].data = phys_addrof(config->version);

   items[2].name = "system.cmdFlags";
   items[2].dataType = UCDataType::UnsignedInt;
   items[2].dataSize = 4u;
   items[2].data = phys_addrof(config->cmdFlags);

   items[3].name = "system.default_os_id";
   items[3].dataType = UCDataType::HexBinary;
   items[3].dataSize = 8u;
   items[3].data = phys_addrof(config->default_os_id);

   items[4].name = "system.default_title_id";
   items[4].dataType = UCDataType::HexBinary;
   items[4].dataSize = 8u;
   items[4].data = phys_addrof(config->default_title_id);

   items[5].name = "system.log.enable";
   items[5].dataType = UCDataType::UnsignedInt;
   items[5].dataSize = 4u;
   items[5].data = phys_addrof(config->log.enable);

   items[6].name = "system.log.max_size";
   items[6].dataType = UCDataType::UnsignedInt;
   items[6].dataSize = 4u;
   items[6].data = phys_addrof(config->log.max_size);

   items[7].name = "system.standby.enable";
   items[7].dataType = UCDataType::UnsignedInt;
   items[7].dataSize = 4u;
   items[7].data = phys_addrof(config->standby.enable);

   items[8].name = "system.ramdisk.cache_user_code";
   items[8].dataType = UCDataType::UnsignedInt;
   items[8].dataSize = 4u;
   items[8].data = phys_addrof(config->ramdisk.cache_user_code);

   items[9].name = "system.ramdisk.max_file_size";
   items[9].dataType = UCDataType::UnsignedInt;
   items[9].dataSize = 4u;
   items[9].data = phys_addrof(config->ramdisk.max_file_size);

   items[10].name = "system.ramdisk.cache_delay_ms";
   items[10].dataType = UCDataType::UnsignedInt;
   items[10].dataSize = 4u;
   items[10].data = phys_addrof(config->ramdisk.cache_delay_ms);

   items[11].name = "system.simulated_ppc_mem2_size";
   items[11].dataType = UCDataType::UnsignedInt;
   items[11].dataSize = 4u;
   items[11].data = phys_addrof(config->simulated_ppc_mem2_size);

   items[12].name = "system.dev_mode";
   items[12].dataType = UCDataType::UnsignedInt;
   items[12].dataSize = 4u;
   items[12].data = phys_addrof(config->dev_mode);

   items[13].name = "system.prev_title_id";
   items[13].dataType = UCDataType::HexBinary;
   items[13].dataSize = 8u;
   items[13].data = phys_addrof(config->prev_title_id);

   items[14].name = "system.prev_os_id";
   items[14].dataType = UCDataType::HexBinary;
   items[14].dataSize = 8u;
   items[14].data = phys_addrof(config->prev_os_id);

   items[15].name = "system.default_app_type";
   items[15].dataType = UCDataType::HexBinary;
   items[15].dataSize = 4u;
   items[15].error = static_cast<UCError>(0x90000001); // whyy???
   items[15].data = phys_addrof(config->default_app_type);

   items[16].name = "system.default_device_type";
   items[16].dataType = UCDataType::String;
   items[16].dataSize = 16u;
   items[16].data = phys_addrof(config->default_device_type);

   items[17].name = "system.default_device_index";
   items[17].dataType = UCDataType::UnsignedInt;
   items[17].dataSize = 4u;
   items[17].data = phys_addrof(config->default_device_index);

   items[18].name = "system.fast_relaunch_value";
   items[18].dataType = UCDataType::UnsignedInt;
   items[18].dataSize = 4u;
   items[18].data = phys_addrof(config->fast_relaunch_value);

   items[19].name = "system.default_eco_title_id";
   items[19].dataType = UCDataType::HexBinary;
   items[19].dataSize = 8u;
   items[19].data = phys_addrof(config->default_eco_title_id);

   auto error = readConfigItems(items, items.size());
   if (error < MCPError::OK || config->version < 21) {
      // Factory reset items
      std::memset(config.getRawPointer(), 0, sizeof(SysProdConfig));
      config->version = 21u;
      config->dev_mode = 0u;
      config->default_eco_title_id = 0x0005001010066000ull;
      config->standby.enable = 0u;
      config->ramdisk.max_file_size = 0xA00000u;
      config->simulated_ppc_mem2_size = 0u;
      config->default_app_type = 0x90000001u;

      // Try save them to file
      deleteConfigItems(items, 1);
      writeConfigItems(items, items.size());
   }

   return MCPError::OK;
}

MCPError
loadSysProdConfig()
{
   StackArray<UCItem, 11> items;
   auto config = phys_addrof(sData->sysProdConfig);

   items[0].name = "slc:sys_prod";
   items[0].access = 0x510u;
   items[0].dataType = UCDataType::Complex;

   items[1].name = "slc:sys_prod.version";
   items[1].access = 0x510u;
   items[1].dataType = UCDataType::UnsignedInt;
   items[1].dataSize = 4u;
   items[1].data = phys_addrof(config->version);

   items[2].name = "slc:sys_prod.eeprom_version";
   items[2].access = 0x510u;
   items[2].dataType = UCDataType::UnsignedShort;
   items[2].dataSize = 2u;
   items[2].data = phys_addrof(config->eeprom_version);

   items[3].name = "slc:sys_prod.product_area";
   items[3].access = 0x510u;
   items[3].dataType = UCDataType::UnsignedInt;
   items[3].dataSize = 4u;
   items[3].data = phys_addrof(config->product_area);

   items[4].name = "slc:sys_prod.game_region";
   items[4].access = 0x510u;
   items[4].dataType = UCDataType::UnsignedInt;
   items[4].dataSize = 4u;
   items[4].data = phys_addrof(config->game_region);

   items[5].name = "slc:sys_prod.ntsc_pal";
   items[5].access = 0x510u;
   items[5].dataType = UCDataType::String;
   items[5].dataSize = 5u;
   items[5].data = phys_addrof(config->ntsc_pal);

   items[6].name = "slc:sys_prod.5ghz_country_code";
   items[6].access = 0x510u;
   items[6].dataType = UCDataType::String;
   items[6].dataSize = 4u;
   items[6].data = phys_addrof(config->wifi_5ghz_country_code);

   items[7].name = "slc:sys_prod.5ghz_country_code_revision";
   items[7].access = 0x510u;
   items[7].dataType = UCDataType::UnsignedByte;
   items[7].dataSize = 1u;
   items[7].data = phys_addrof(config->wifi_5ghz_country_code_revision);

   items[8].name = "slc:sys_prod.code_id";
   items[8].access = 0x510u;
   items[8].dataType = UCDataType::String;
   items[8].dataSize = 8u;
   items[8].data = phys_addrof(config->code_id);

   items[9].name = "slc:sys_prod.serial_id";
   items[9].access = 0x510u;
   items[9].dataType = UCDataType::String;
   items[9].dataSize = 12u;
   items[9].data = phys_addrof(config->serial_id);

   items[10].name = "slc:sys_prod.model_number";
   items[10].access = 0x510u;
   items[10].dataType = UCDataType::String;
   items[10].dataSize = 16u;
   items[10].data = phys_addrof(config->model_number);

   auto error = readConfigItems(items, items.size());
   if (error < MCPError::OK || config->version <= 4) {
      // Factory reset items
      std::memset(config.getRawPointer(), 0, sizeof(SysProdConfig));
      config->version = 5u;
      config->game_region = MCPRegion::Europe;
      config->wifi_5ghz_country_code = "EU";
      config->wifi_5ghz_country_code_revision = uint8_t { 24 };
      config->ntsc_pal = "PAL";
      config->code_id = "FEM";
      config->serial_id = std::to_string(100000000 | 0xDECAF);
      config->model_number = "WUP-101(03)";

      // Try save them to file
      deleteConfigItems(items, 1);
      writeConfigItems(items, items.size());
   }

   return MCPError::OK;
}

phys_ptr<RtcConfig>
getRtcConfig()
{
   return phys_addrof(sData->rtcConfig);
}

phys_ptr<SystemConfig>
getSystemConfig()
{
   return phys_addrof(sData->systemConfig);
}

phys_ptr<SysProdConfig>
getSysProdConfig()
{
   return phys_addrof(sData->sysProdConfig);
}

void
initialiseStaticConfigData()
{
   sData = phys_cast<StaticConfigData>(allocProcessStatic(sizeof(StaticConfigData)));
}

} // namespace ios::mcp::internal
