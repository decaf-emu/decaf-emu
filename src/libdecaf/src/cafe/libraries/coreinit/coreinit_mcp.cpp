#include "coreinit.h"
#include "coreinit_fsa.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mcp.h"
#include "cafe/cafe_stackobject.h"
#include "ios/ios_error.h"

namespace cafe::coreinit
{

using ios::mcp::MCPCommand;
using ios::mcp::MCPResponseGetTitleId;
using ios::mcp::MCPResponseGetOwnTitleInfo;
using ios::mcp::MCPRequestGetOwnTitleInfo;
using ios::mcp::MCPRequestSearchTitleList;
using ios::mcp::MCPTitleListSearchFlags;

static constexpr uint32_t SmallMessageCount = 0x100;
static constexpr uint32_t SmallMessageSize = 0x80;

static constexpr uint32_t LargeMessageCount = 4;
static constexpr uint32_t LargeMessageSize = 0x1000;

struct StaticMcpData
{
   be2_val<BOOL> initialised = FALSE;

   be2_virt_ptr<IPCBufPool> smallMessagePool = nullptr;
   be2_virt_ptr<IPCBufPool> largeMessagePool = nullptr;

   be2_array<uint8_t, SmallMessageCount * SmallMessageSize> smallMessageBuffer;
   be2_array<uint8_t, LargeMessageCount * LargeMessageSize> largeMessageBuffer;

   be2_val<uint32_t> smallMessageCount = 0;
   be2_val<uint32_t> largeMessageCount = 0;
};


static virt_ptr<StaticMcpData>
sMcpData = nullptr;


IOSError
MCP_Open()
{
   return IOS_Open(make_stack_string("/dev/mcp"), IOSOpenMode::None);
}


void
MCP_Close(IOSHandle handle)
{
   IOS_Close(handle);
}


MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    virt_ptr<MCPTitleListType> titleInfo)
{
   auto result = MCPError::OK;
   auto request = virt_cast<MCPRequestGetOwnTitleInfo *>(
                     internal::mcpAllocateMessage(sizeof(MCPRequestGetOwnTitleInfo)));

   if (!request) {
      return MCPError::Alloc;
   }

   auto response = virt_cast<MCPResponseGetOwnTitleInfo *>(
                      internal::mcpAllocateMessage(sizeof(MCPResponseGetOwnTitleInfo)));

   if (!response) {
      internal::mcpFreeMessage(request);
      return MCPError::Alloc;
   }

   // TODO: __KernelGetInfo(0, &request->unk0x00, 0xA8, 0);
   request->unk0x00 = 0u;

   auto iosError = IOS_Ioctl(handle,
                             MCPCommand::GetOwnTitleInfo,
                             request,
                             sizeof(uint32_t),
                             response,
                             sizeof(MCPResponseGetOwnTitleInfo));

   result = internal::mcpDecodeIosErrorToMcpError(iosError);

   if (result >= 0) {
      std::memcpy(titleInfo.getRawPointer(),
                  virt_addrof(response->titleInfo).getRawPointer(),
                  sizeof(MCPTitleListType));
   }

   internal::mcpFreeMessage(request);
   internal::mcpFreeMessage(response);
   return result;
}


MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       virt_ptr<MCPSysProdSettings> settings)
{
   if (!settings) {
      return MCPError::InvalidParam;
   }

   auto message = internal::mcpAllocateMessage(sizeof(IOSVec));

   if (!message) {
      return MCPError::Alloc;
   }

   auto outVecs = virt_cast<IOSVec *>(message);
   outVecs[0].vaddr = virt_cast<virt_addr>(settings);
   outVecs[0].len = static_cast<uint32_t>(sizeof(MCPSysProdSettings));

   auto iosError = IOS_Ioctlv(handle, MCPCommand::GetSysProdSettings, 0, 1, outVecs);
   auto mcpError = internal::mcpDecodeIosErrorToMcpError(iosError);

   internal::mcpFreeMessage(message);
   return mcpError;
}


MCPError
MCP_GetTitleId(IOSHandle handle,
               virt_ptr<uint64_t> outTitleId)
{
   auto result = MCPError::OK;
   auto output = internal::mcpAllocateMessage(sizeof(MCPResponseGetTitleId));

   if (!output) {
      return MCPError::Alloc;
   }

   auto iosError = IOS_Ioctl(handle,
                             MCPCommand::GetTitleId,
                             nullptr,
                             0,
                             output,
                             sizeof(MCPResponseGetTitleId));

   result = internal::mcpDecodeIosErrorToMcpError(iosError);

   if (result >= 0) {
      auto response = virt_cast<MCPResponseGetTitleId *>(output);
      *outTitleId = response->titleId;
   }

   internal::mcpFreeMessage(output);
   return result;
}


MCPError
MCP_GetTitleInfo(IOSHandle handle,
                 uint64_t titleId,
                 virt_ptr<MCPTitleListType> titleInfo)
{
   StackObject<MCPTitleListType> searchTitle;
   std::memset(searchTitle.getRawPointer(), 0, sizeof(MCPTitleListType));
   searchTitle->titleId = titleId;

   auto iosError = internal::mcpSearchTitleList(handle,
                                                searchTitle,
                                                MCPTitleListSearchFlags::TitleId,
                                                titleInfo,
                                                1);

   if (iosError != 1) {
      return MCPError::System;
   }

   return MCPError::OK;
}


MCPError
MCP_TitleCount(IOSHandle handle)
{
   auto result = IOS_Ioctl(handle,
                           MCPCommand::TitleCount,
                           nullptr, 0,
                           nullptr, 0);

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   return static_cast<MCPError>(result);
}


MCPError
MCP_TitleList(IOSHandle handle,
              virt_ptr<uint32_t> outTitleCount,
              virt_ptr<MCPTitleListType> titleList,
              uint32_t titleListSizeBytes)
{
   auto result = IOSError::OK;

   if (!titleList || !titleListSizeBytes) {
      result = static_cast<IOSError>(MCP_TitleCount(handle));
   } else {
      StackObject<MCPTitleListType> searchTitle;
      std::memset(searchTitle.getRawPointer(), 0, sizeof(MCPTitleListType));

      result = internal::mcpSearchTitleList(handle,
                                            searchTitle,
                                            MCPTitleListSearchFlags::None,
                                            titleList,
                                            titleListSizeBytes / sizeof(MCPTitleListType));
   }

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *outTitleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByAppType(IOSHandle handle,
                       MCPAppType appType,
                       virt_ptr<uint32_t> outTitleCount,
                       virt_ptr<MCPTitleListType> titleList,
                       uint32_t titleListSizeBytes)
{
   StackObject<MCPTitleListType> searchTitle;
   std::memset(searchTitle.getRawPointer(), 0, sizeof(MCPTitleListType));
   searchTitle->appType = appType;

   auto result = internal::mcpSearchTitleList(handle,
                                              searchTitle,
                                              MCPTitleListSearchFlags::AppType,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *outTitleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByUniqueId(IOSHandle handle,
                        uint32_t uniqueId,
                        virt_ptr<uint32_t> outTitleCount,
                        virt_ptr<MCPTitleListType> titleList,
                        uint32_t titleListSizeBytes)
{
   StackObject<MCPTitleListType> searchTitle;
   std::memset(searchTitle.getRawPointer(), 0, sizeof(MCPTitleListType));
   searchTitle->titleId = uniqueId << 8;
   searchTitle->appType = MCPAppType::Unk0x0800000E;

   auto searchFlags = MCPTitleListSearchFlags::UniqueId
                    | MCPTitleListSearchFlags::AppType;

   auto result = internal::mcpSearchTitleList(handle,
                                              searchTitle,
                                              searchFlags,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *outTitleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType(IOSHandle handle,
                                                  uint32_t uniqueId,
                                                  virt_ptr<const char> indexedDevice,
                                                  uint8_t unk0x60,
                                                  MCPAppType appType,
                                                  virt_ptr<uint32_t> outTitleCount,
                                                  virt_ptr<MCPTitleListType> titleList,
                                                  uint32_t titleListSizeBytes)
{
   StackObject<MCPTitleListType> searchTitle;
   std::memset(searchTitle.getRawPointer(), 0, sizeof(MCPTitleListType));
   searchTitle->titleId = uniqueId << 8;
   searchTitle->appType = appType;
   searchTitle->unk0x60 = unk0x60;
   std::memcpy(virt_addrof(searchTitle->indexedDevice).getRawPointer(),
               indexedDevice.getRawPointer(),
               4);

   auto searchFlags = MCPTitleListSearchFlags::UniqueId
                    | MCPTitleListSearchFlags::AppType
                    | MCPTitleListSearchFlags::Unk0x60
                    | MCPTitleListSearchFlags::IndexedDevice;

   auto result = internal::mcpSearchTitleList(handle,
                                              searchTitle,
                                              searchFlags,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *outTitleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


namespace internal
{


virt_ptr<void>
mcpAllocateMessage(uint32_t size)
{
   auto message = virt_ptr<void> { nullptr };

   if (size == 0) {
      return nullptr;
   } else if (size <= SmallMessageSize) {
      message = IPCBufPoolAllocate(sMcpData->smallMessagePool, size);
   } else {
      message = IPCBufPoolAllocate(sMcpData->largeMessagePool, size);
   }

   std::memset(message.getRawPointer(), 0, size);
   return message;
}


MCPError
mcpFreeMessage(virt_ptr<void> message)
{
   if (IPCBufPoolFree(sMcpData->smallMessagePool, message) == IOSError::OK) {
      return MCPError::OK;
   }

   if (IPCBufPoolFree(sMcpData->largeMessagePool, message) == IOSError::OK) {
      return MCPError::OK;
   }

   return MCPError::Opcode;
}


MCPError
mcpDecodeIosErrorToMcpError(IOSError error)
{
   auto category = ios::getErrorCategory(error);
   auto code = ios::getErrorCode(error);
   auto mcpError = static_cast<MCPError>(error);

   if (error < 0) {
      switch (category) {
      case IOSErrorCategory::Kernel:
         if (code > -1000) {
            mcpError = static_cast<MCPError>(code + MCPError::KernelErrorBase);
         } else if(code < -1999) {
            mcpError = static_cast<MCPError>(code - (IOSErrorCategory::MCP << 16));
         }
         break;
      case IOSErrorCategory::FSA:
         if (code == FSAStatus::AlreadyOpen) {
            mcpError = MCPError::AlreadyOpen;
         } else if (code == FSAStatus::DataCorrupted) {
            mcpError = MCPError::DataCorrupted;
         } else if (code == FSAStatus::StorageFull) {
            mcpError = MCPError::StorageFull;
         } else if (code == FSAStatus::WriteProtected) {
            mcpError = MCPError::WriteProtected;
         } else {
            mcpError = static_cast<MCPError>(error + 0xFFFF0000 - 4000);
         }
         break;
      case IOSErrorCategory::MCP:
         mcpError = static_cast<MCPError>(error);
         break;
      }
   }

   return mcpError;
}


IOSError
mcpSearchTitleList(IOSHandle handle,
                   virt_ptr<MCPTitleListType> searchTitle,
                   MCPTitleListSearchFlags searchFlags,
                   virt_ptr<MCPTitleListType> titleList,
                   uint32_t titleListLength)
{
   auto message = mcpAllocateMessage(sizeof(MCPRequestSearchTitleList));

   if (!message) {
      return static_cast<IOSError>(MCPError::Alloc);
   }

   auto request = virt_cast<MCPRequestSearchTitleList *>(message);
   request->searchTitle = *searchTitle;
   request->searchFlags = searchFlags;

   auto iosError = IOS_Ioctl(handle,
                             MCPCommand::SearchTitleList,
                             request,
                             sizeof(MCPRequestSearchTitleList),
                             titleList,
                             titleListLength * sizeof(MCPTitleListType));

   mcpFreeMessage(message);
   return iosError;
}


void
initialiseMcp()
{
   sMcpData->smallMessagePool = IPCBufPoolCreate(virt_addrof(sMcpData->smallMessageBuffer),
                                                 static_cast<uint32_t>(sMcpData->smallMessageBuffer.size()),
                                                 SmallMessageSize,
                                                 virt_addrof(sMcpData->smallMessageCount),
                                                 1);

   sMcpData->largeMessagePool = IPCBufPoolCreate(virt_addrof(sMcpData->largeMessageBuffer),
                                                 static_cast<uint32_t>(sMcpData->largeMessageBuffer.size()),
                                                 LargeMessageSize,
                                                 virt_addrof(sMcpData->largeMessageCount),
                                                 1);

   sMcpData->initialised = true;
}

} // namespace internal

void
Library::registerMcpSymbols()
{
   RegisterFunctionExport(MCP_Open);
   RegisterFunctionExport(MCP_Close);
   RegisterFunctionExport(MCP_GetOwnTitleInfo);
   RegisterFunctionExport(MCP_GetSysProdSettings);
   RegisterFunctionExport(MCP_GetTitleId);
   RegisterFunctionExport(MCP_GetTitleInfo);
   RegisterFunctionExport(MCP_TitleCount);
   RegisterFunctionExport(MCP_TitleList);
   RegisterFunctionExport(MCP_TitleListByAppType);
   RegisterFunctionExport(MCP_TitleListByUniqueId);
   RegisterFunctionExport(MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType);

   RegisterDataInternal(sMcpData);
}

} // namespace cafe::coreinit
