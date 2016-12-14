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
