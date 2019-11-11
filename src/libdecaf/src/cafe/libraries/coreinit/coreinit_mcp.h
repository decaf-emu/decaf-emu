#pragma once
#include "coreinit_enum.h"
#include "coreinit_ios.h"
#include "ios/mcp/ios_mcp_mcp.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_mcp MCP
 * \ingroup coreinit
 * @{
 */

using ios::mcp::MCPAppType;
using ios::mcp::MCPCountryCode;
using ios::mcp::MCPDevice;
using ios::mcp::MCPError;
using ios::mcp::MCPRegion;
using ios::mcp::MCPSysProdSettings;
using ios::mcp::MCPTitleListType;
using ios::mcp::MCPTitleListSearchFlags;
using ios::mcp::MCPUpdateProgress;

IOSError
MCP_Open();

void
MCP_Close(IOSHandle handle);

IOSError
MCP_DeviceList(IOSHandle handle,
               virt_ptr<int32_t> numDevices,
               virt_ptr<MCPDevice> deviceList,
               uint32_t deviceListSizeBytes);

IOSError
MCP_FullDeviceList(IOSHandle handle,
                   virt_ptr<int32_t> numDevices,
                   virt_ptr<MCPDevice> deviceList,
                   uint32_t deviceListSizeBytes);

int32_t
MCP_GetErrorCodeForViewer(MCPError error);

MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    virt_ptr<MCPTitleListType> titleInfo);

MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       virt_ptr<MCPSysProdSettings> settings);

MCPError
MCP_GetTitleId(IOSHandle handle,
               virt_ptr<uint64_t> outTitleId);

MCPError
MCP_GetTitleInfo(IOSHandle handle,
                 uint64_t titleId,
                 virt_ptr<MCPTitleListType> titleInfo);

MCPError
MCP_TitleCount(IOSHandle handle);

MCPError
MCP_TitleList(IOSHandle handle,
              virt_ptr<uint32_t> outTitleCount,
              virt_ptr<MCPTitleListType> titleList,
              uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByAppType(IOSHandle handle,
                       MCPAppType appType,
                       virt_ptr<uint32_t> outTitleCount,
                       virt_ptr<MCPTitleListType> titleList,
                       uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueId(IOSHandle handle,
                        uint32_t uniqueId,
                        virt_ptr<uint32_t> outTitleCount,
                        virt_ptr<MCPTitleListType> titleList,
                        uint32_t titleListSizeBytes);

MCPError
MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType(IOSHandle handle,
                                                  uint32_t uniqueId,
                                                  virt_ptr<const char> indexedDevice,
                                                  uint8_t unk0x60,
                                                  MCPAppType appType,
                                                  virt_ptr<uint32_t> outTitleCount,
                                                  virt_ptr<MCPTitleListType> titleList,
                                                  uint32_t titleListSizeBytes);

MCPError
MCP_UpdateCheckContext(IOSHandle handle,
                       virt_ptr<uint32_t> outResult);

MCPError
MCP_UpdateCheckResume(IOSHandle handle,
                      virt_ptr<uint32_t> outResult);

MCPError
MCP_UpdateGetProgress(IOSHandle handle,
                      virt_ptr<MCPUpdateProgress> outUpdateProgress);

namespace internal
{

void
initialiseMcp();

virt_ptr<void>
mcpAllocateMessage(uint32_t size);

MCPError
mcpFreeMessage(virt_ptr<void> message);

MCPError
mcpDecodeIosErrorToMcpError(IOSError error);

IOSError
mcpSearchTitleList(IOSHandle handle,
                   virt_ptr<MCPTitleListType> searchTitle,
                   MCPTitleListSearchFlags searchFlags,
                   virt_ptr<MCPTitleListType> titleListOut,
                   uint32_t titleListLength);

} // namespace internal

/** @} */

} // namespace cafe::coreinit
