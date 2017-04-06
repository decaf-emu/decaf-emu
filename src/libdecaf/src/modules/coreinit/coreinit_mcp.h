#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"

#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_mcp MCP
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct MCPSysProdSettings
{
   UNKNOWN(3);
   be_val<SCIRegion> platformRegion;
   UNKNOWN(0x7);
   be_val<SCIRegion> gameRegion;
   UNKNOWN(0x3A);
};
CHECK_OFFSET(MCPSysProdSettings, 0x03, platformRegion);
CHECK_OFFSET(MCPSysProdSettings, 0x0B, gameRegion);
CHECK_SIZE(MCPSysProdSettings, 0x46);

struct MCPTitleListType
{
   be_val<uint64_t> titleId;
   UNKNOWN(4);
   char path[56];
   be_val<MCPAppType> appType;
   UNKNOWN(0x54 - 0x48);
   uint8_t device;
   UNKNOWN(1);
   char indexedDevice[10];
   uint8_t unk0x60;
};
CHECK_OFFSET(MCPTitleListType, 0x00, titleId);
CHECK_OFFSET(MCPTitleListType, 0x0C, path);
CHECK_OFFSET(MCPTitleListType, 0x44, appType);
CHECK_OFFSET(MCPTitleListType, 0x54, device);
CHECK_OFFSET(MCPTitleListType, 0x56, indexedDevice);
CHECK_OFFSET(MCPTitleListType, 0x60, unk0x60);
CHECK_SIZE(MCPTitleListType, 0x61);

#pragma pack(pop)

IOSError
MCP_Open();

void
MCP_Close(IOSHandle handle);

MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    MCPTitleListType *titleInfo);

MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       MCPSysProdSettings *settings);

MCPError
MCP_GetTitleId(IOSHandle handle,
               be_val<uint64_t> *titleId);

MCPError
MCP_GetTitleInfo(IOSHandle handle,
                 uint64_t titleId,
                 MCPTitleListType *titleInfo);

MCPError
MCP_TitleCount(IOSHandle handle);

MCPError
MCP_TitleList(IOSHandle handle,
              uint32_t *titleCount,
              MCPTitleListType *titleList,
              uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByAppType(IOSHandle handle,
                       MCPAppType appType,
                       uint32_t *titleCount,
                       MCPTitleListType *titleList,
                       uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueId(IOSHandle handle,
                        uint32_t uniqueId,
                        uint32_t *titleCount,
                        MCPTitleListType *titleList,
                        uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType(IOSHandle handle,
                                                  uint32_t uniqueId,
                                                  const char *indexedDevice,
                                                  uint8_t unk0x60,
                                                  MCPAppType appType,
                                                  uint32_t *titleCount,
                                                  MCPTitleListType *titleList,
                                                  uint32_t titleListSizeBytes);

namespace internal
{

void
mcpInit();

void *
mcpAllocateMessage(uint32_t size);

MCPError
mcpFreeMessage(void *message);

MCPError
mcpDecodeIosErrorToMcpError(IOSError error);

IOSError
mcpSearchTitleList(IOSHandle handle,
                   MCPTitleListType *searchTitle,
                   MCPTitleListSearchFlags searchFlags,
                   MCPTitleListType *titleListOut,
                   uint32_t titleListLength);

} // namespace internal

/** @} */

} // namespace coreinit
