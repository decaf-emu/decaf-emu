#ifndef IOS_MCP_ENUM_H
#define IOS_MCP_ENUM_H

#include <common/enum_start.h>

ENUM_NAMESPACE_BEG(kernel)

ENUM_NAMESPACE_BEG(ios)

ENUM_NAMESPACE_BEG(mcp)

ENUM_BEG(MCPAppType, uint32_t)
   ENUM_VALUE(Unk0x0800000E,        0x0800000E)
ENUM_END(MCPAppType)

ENUM_BEG(MCPCommand, uint32_t)
   ENUM_VALUE(GetSysProdSettings,   0x40)
   ENUM_VALUE(GetOwnTitleInfo,      0x4C)
   ENUM_VALUE(TitleCount,           0x4D)
   ENUM_VALUE(SearchTitleList,      0x58)
   ENUM_VALUE(GetTitleId,           0x5B)
ENUM_END(MCPCommand)

ENUM_BEG(MCPCountryCode, uint32_t)
   ENUM_VALUE(USA,                  0x31)
   ENUM_VALUE(UnitedKingdom,        0x63)
ENUM_END(MCPCountryCode)

ENUM_BEG(MCPError, int32_t)
   ENUM_VALUE(OK,                   0)
   ENUM_VALUE(AllocError,           -0x40003)
   ENUM_VALUE(InvalidOp,            -0x40004)
   ENUM_VALUE(InvalidArg,           -0x40005)
   ENUM_VALUE(InvalidType,          -0x40006)
   ENUM_VALUE(Unsupported,          -0x40007)
   ENUM_VALUE(AlreadyOpen,          -0x40054)
   ENUM_VALUE(StorageFull,          -0x40055)
   ENUM_VALUE(WriteProtected,       -0x40056)
   ENUM_VALUE(DataCorrupted,        -0x40057)
   ENUM_VALUE(KernelErrorBase,      -0x403E8)
ENUM_END(MCPError)

ENUM_BEG(MCPLanguage, uint32_t)
   ENUM_VALUE(Japanese,             0x00)
   ENUM_VALUE(English,              0x01)
   ENUM_VALUE(French,               0x02)
   ENUM_VALUE(German,               0x03)
   ENUM_VALUE(Italian,              0x04)
   ENUM_VALUE(Spanish,              0x05)
   ENUM_VALUE(Chinese,              0x06)
   ENUM_VALUE(Korean,               0x07)
   ENUM_VALUE(Dutch,                0x08)
   ENUM_VALUE(Portugese,            0x09)
   ENUM_VALUE(Russian,              0x0A)
   ENUM_VALUE(Taiwanese,            0x0B)
   ENUM_VALUE(Max,                  0x0C)
ENUM_END(MCPLanguage)

ENUM_BEG(MCPRegion, uint8_t)
   ENUM_VALUE(JAP,                  0x01)
   ENUM_VALUE(USA,                  0x02)
   ENUM_VALUE(EUR,                  0x04)
ENUM_END(MCPRegion)

FLAGS_BEG(MCPTitleListSearchFlags, uint32_t)
   FLAGS_VALUE(None,                 0)
   FLAGS_VALUE(TitleId,              1 << 0)
   FLAGS_VALUE(AppType,              1 << 2)
   FLAGS_VALUE(IndexedDevice,        1 << 6)
   FLAGS_VALUE(Unk0x60,              1 << 8)
   FLAGS_VALUE(Path,                 1 << 9)
   FLAGS_VALUE(UniqueId,             1 << 10)
FLAGS_END(MCPTitleListSearchFlags)

ENUM_NAMESPACE_END(mcp)

ENUM_NAMESPACE_END(ios)

ENUM_NAMESPACE_END(kernel)

#include <common/enum_end.h>

#endif // ifdef IOS_MCP_ENUM_H
