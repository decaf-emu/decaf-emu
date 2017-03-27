#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace decaf
{

namespace Language_
{
enum Values
{
   Japanese,
   English,
   French,
   German,
   Italian,
   Spanish,
   Chinese,
   Korean,
   Dutch,
   Portugese,
   Russian,
   Taiwanese,
   Max,
};
}
using Language = Language_::Values;

struct MetaXML
{
   unsigned version;
   std::string product_code;
   uint64_t os_version;
   unsigned title_version;
   uint64_t title_id;
   uint64_t common_save_size;
   uint64_t account_save_size;
   uint64_t common_boss_size;
   uint64_t account_boss_size;
   unsigned olv_accesskey;
   unsigned region;
   std::array<std::string, Language::Max> longnames;
   std::array<std::string, Language::Max> shortnames;
   std::array<std::string, Language::Max> publishers;
};

struct AppXML
{
   unsigned version;
   uint64_t os_version;
   uint64_t title_id;
   unsigned title_version;
   unsigned sdk_version;
   unsigned app_type;
   unsigned group_id;
};

struct CosXML
{
   enum PermissionGroup
   {
      BSP = 1,
      DK = 3,
      USB = 9,
      FS = 11,
      UHS = 12,
      MCP = 13,
      NIM = 14,
      ACT = 15,
      FPD = 16,
      BOSS = 17,
      ACP = 18,
      AC = 20,
      NDM = 21,
      NSEC = 22,
   };

   enum FSPermission
   {
      SdCardRead = 0x90000,
      SdCardWrite = 0xA0000,
   };

   enum MCPPermission
   {
      AddOnContent = 0x80,
      SciErrorLog = 0x200,
   };

   unsigned version;
   unsigned cmdFlags;
   std::string argstr;
   unsigned avail_size;
   unsigned codegen_size;
   unsigned codegen_core;
   unsigned max_size;
   unsigned max_codesize;
   unsigned default_stack0_size;
   unsigned default_stack1_size;
   unsigned default_stack2_size;
   unsigned exception_stack0_size;
   unsigned exception_stack1_size;
   unsigned exception_stack2_size;
   uint64_t permission_fs;
   uint64_t permission_mcp;
};

struct GameInfo
{
   AppXML app;
   CosXML cos;
   MetaXML meta;
};

const GameInfo &
getGameInfo();

} // namespace decaf
