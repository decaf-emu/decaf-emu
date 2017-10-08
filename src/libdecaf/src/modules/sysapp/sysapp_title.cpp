#include "sysapp.h"
#include "sysapp_title.h"
#include "modules/coreinit/coreinit_mcp.h"
#include "modules/coreinit/coreinit_enum_string.h"
#include "ppcutils/stackobject.h"

#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace sysapp
{

static const uint64_t
sSysAppTitleId[][3] =
{
   {
      // Updater
      0x0005001010040000ull,
      0x0005001010040100ull,
      0x0005001010040200ull,
   },

   {
      // System Settings
      0x0005001010047000ull,
      0x0005001010047100ull,
      0x0005001010047200ull,
   },

   {
      // Parental Controls
      0x0005001010048000ull,
      0x0005001010048100ull,
      0x0005001010048200ull,
   },

   {
      // User Settings
      0x0005001010049000ull,
      0x0005001010049100ull,
      0x0005001010049200ull,
   },

   {
      // Mii Maker
      0x000500101004A000ull,
      0x000500101004A100ull,
      0x000500101004A200ull,
   },

   {
      // Account Settings
      0x000500101004B000ull,
      0x000500101004B100ull,
      0x000500101004B200ull,
   },

   {
      // Daily log
      0x000500101004C000ull,
      0x000500101004C100ull,
      0x000500101004C200ull,
   },

   {
      // Notifications
      0x000500101004D000ull,
      0x000500101004D100ull,
      0x000500101004D200ull,
   },

   {
      // Health and Safety Information
      0x000500101004E000ull,
      0x000500101004E100ull,
      0x000500101004E200ull,
   },

   {
      // Electronic Manual
      0x0005001B10059000ull,
      0x0005001B10059100ull,
      0x0005001B10059200ull,
   },

   {
      // Wii U Chat
      0x000500101005A000ull,
      0x000500101005A100ull,
      0x000500101005A200ull,
   },

   {
      // "Software/Data Transfer"
      0x0005001010062000ull,
      0x0005001010062100ull,
      0x0005001010062200ull,
   },
};

uint64_t
SYSGetSystemApplicationTitleId(SystemAppId id)
{
   ppcutils::StackObject<coreinit::MCPSysProdSettings> settings;
   decaf_check(id < SystemAppId::Max);

   auto mcp = coreinit::MCP_Open();
   coreinit::MCP_GetSysProdSettings(mcp, settings);
   coreinit::MCP_Close(mcp);

   auto region = settings->platformRegion;

   if (!region) {
      region = coreinit::MCPRegion::USA;
   }

   return SYSGetSystemApplicationTitleIdByProdArea(id, region);
}

uint64_t
SYSGetSystemApplicationTitleIdByProdArea(SystemAppId id,
                                         coreinit::MCPRegion region)
{
   auto regionIdx = 1u;

   if (region == coreinit::MCPRegion::Japan) {
      regionIdx = 0u;
   } else if (region == coreinit::MCPRegion::USA) {
      regionIdx = 1u;
   } else if (region == coreinit::MCPRegion::Europe) {
      regionIdx = 2u;
   } else {
      be_val<uint8_t> customRegion = (uint8_t)region;
      decaf_abort(fmt::format("Unknown region {}", customRegion));
      //decaf_abort(fmt::format("Unknown region {}", region));
   }

   return sSysAppTitleId[id][regionIdx];
}

void
Module::registerTitleFunctions()
{
   RegisterKernelFunctionName("_SYSGetSystemApplicationTitleId", SYSGetSystemApplicationTitleId);
   RegisterKernelFunctionName("_SYSGetSystemApplicationTitleIdByProdArea", SYSGetSystemApplicationTitleIdByProdArea);
}

} // namespace sysapp
