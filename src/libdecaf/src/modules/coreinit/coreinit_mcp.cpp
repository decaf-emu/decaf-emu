#include "coreinit.h"
#include "coreinit_fsa.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mcp.h"
#include "ios/ios_error.h"

#include <common/decaf_assert.h>
#include <ppcutils/stackobject.h>

namespace coreinit
{

using ios::mcp::MCPCommand;
using ios::mcp::MCPResponseGetTitleId;
using ios::mcp::MCPResponseGetOwnTitleInfo;
using ios::mcp::MCPRequestGetOwnTitleInfo;
using ios::mcp::MCPRequestSearchTitleList;
using ios::mcp::MCPTitleListSearchFlags;

struct MCPData
{
   static constexpr uint32_t SmallMessageCount = 0x100;
   static constexpr uint32_t SmallMessageSize = 0x80;

   static constexpr uint32_t LargeMessageCount = 4;
   static constexpr uint32_t LargeMessageSize = 0x1000;

   bool initialised;

   IPCBufPool *smallMessagePool;
   IPCBufPool *largeMessagePool;

   std::array<uint8_t, SmallMessageCount * SmallMessageSize> smallMessageBuffer;
   std::array<uint8_t, LargeMessageCount * LargeMessageSize> largeMessageBuffer;

   be_val<uint32_t> smallMessageCount;
   be_val<uint32_t> largeMessageCount;
};


static MCPData *
sMcpData = nullptr;


IOSError
MCP_Open()
{
   return IOS_Open("/dev/mcp", IOSOpenMode::None);
}


void
MCP_Close(IOSHandle handle)
{
   IOS_Close(handle);
}


MCPError
MCP_GetOwnTitleInfo(IOSHandle handle,
                    MCPTitleListType *titleInfo)
{
   auto result = MCPError::OK;
   auto request = reinterpret_cast<MCPRequestGetOwnTitleInfo *>(
                     internal::mcpAllocateMessage(sizeof(MCPRequestGetOwnTitleInfo)));

   if (!request) {
      return MCPError::Alloc;
   }

   auto response = reinterpret_cast<MCPResponseGetOwnTitleInfo *>(
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
      std::memcpy(titleInfo,
                  virt_addrof(response->titleInfo).getRawPointer(),
                  sizeof(MCPTitleListType));
   }

   internal::mcpFreeMessage(request);
   internal::mcpFreeMessage(response);
   return result;
}


MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       MCPSysProdSettings *settings)
{
   if (!settings) {
      return MCPError::InvalidParam;
   }

   auto message = internal::mcpAllocateMessage(sizeof(IOSVec));

   if (!message) {
      return MCPError::Alloc;
   }

   auto outVecs = reinterpret_cast<IOSVec *>(message);
   outVecs[0].vaddr = cpu::translate(settings);
   outVecs[0].len = static_cast<uint32_t>(sizeof(MCPSysProdSettings));

   auto iosError = IOS_Ioctlv(handle, MCPCommand::GetSysProdSettings, 0, 1, outVecs);
   auto mcpError = internal::mcpDecodeIosErrorToMcpError(iosError);

   internal::mcpFreeMessage(message);
   return mcpError;
}


MCPError
MCP_GetTitleId(IOSHandle handle,
               be_val<uint64_t> *titleId)
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
      auto response = reinterpret_cast<MCPResponseGetTitleId *>(output);
      *titleId = response->titleId;
   }

   internal::mcpFreeMessage(output);
   return result;
}


MCPError
MCP_GetTitleInfo(IOSHandle handle,
                 uint64_t titleId,
                 MCPTitleListType *titleInfo)
{
   auto searchTitle = MCPTitleListType { 0 };
   searchTitle.titleId = titleId;

   auto iosError = internal::mcpSearchTitleList(handle,
                                                &searchTitle,
                                                MCPTitleListSearchFlags::TitleId,
                                                titleInfo,
                                                1);

   if (iosError != 1) {
      // TODO: Name error code 0xFFFBFFFE.
      return static_cast<MCPError>(0xFFFBFFFE);
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
              uint32_t *titleCount,
              MCPTitleListType *titleList,
              uint32_t titleListSizeBytes)
{
   auto result = IOSError::OK;

   if (!titleList || !titleListSizeBytes) {
      result = static_cast<IOSError>(MCP_TitleCount(handle));
   } else {
      auto searchTitle = MCPTitleListType { 0 };

      result = internal::mcpSearchTitleList(handle,
                                            &searchTitle,
                                            MCPTitleListSearchFlags::None,
                                            titleList,
                                            titleListSizeBytes / sizeof(MCPTitleListType));
   }

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *titleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByAppType(IOSHandle handle,
                       MCPAppType appType,
                       uint32_t *titleCount,
                       MCPTitleListType *titleList,
                       uint32_t titleListSizeBytes)
{
   auto searchTitle = MCPTitleListType { 0 };
   searchTitle.appType = appType;

   auto result = internal::mcpSearchTitleList(handle,
                                              &searchTitle,
                                              MCPTitleListSearchFlags::AppType,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *titleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByUniqueId(IOSHandle handle,
                        uint32_t uniqueId,
                        uint32_t *titleCount,
                        MCPTitleListType *titleList,
                        uint32_t titleListSizeBytes)
{
   auto searchTitle = MCPTitleListType { 0 };
   searchTitle.titleId = uniqueId << 8;
   searchTitle.appType = MCPAppType::Unk0x0800000E;

   auto searchFlags = MCPTitleListSearchFlags::UniqueId
                    | MCPTitleListSearchFlags::AppType;

   auto result = internal::mcpSearchTitleList(handle,
                                              &searchTitle,
                                              searchFlags,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *titleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


MCPError
MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType(IOSHandle handle,
                                                  uint32_t uniqueId,
                                                  const char *indexedDevice,
                                                  uint8_t unk0x60,
                                                  MCPAppType appType,
                                                  uint32_t *titleCount,
                                                  MCPTitleListType *titleList,
                                                  uint32_t titleListSizeBytes)
{
   auto searchTitle = MCPTitleListType { 0 };
   searchTitle.titleId = uniqueId << 8;
   searchTitle.appType = appType;
   searchTitle.unk0x60 = unk0x60;
   std::memcpy(virt_addrof(searchTitle.indexedDevice).getRawPointer(),
               indexedDevice,
               4);

   auto searchFlags = MCPTitleListSearchFlags::UniqueId
                    | MCPTitleListSearchFlags::AppType
                    | MCPTitleListSearchFlags::Unk0x60
                    | MCPTitleListSearchFlags::IndexedDevice;

   auto result = internal::mcpSearchTitleList(handle,
                                              &searchTitle,
                                              searchFlags,
                                              titleList,
                                              titleListSizeBytes / sizeof(MCPTitleListType));

   if (result < 0) {
      return internal::mcpDecodeIosErrorToMcpError(result);
   }

   *titleCount = static_cast<uint32_t>(result);
   return MCPError::OK;
}


namespace internal
{

void
mcpInit()
{
   sMcpData->smallMessagePool = IPCBufPoolCreate(sMcpData->smallMessageBuffer.data(),
                                                 static_cast<uint32_t>(sMcpData->smallMessageBuffer.size()),
                                                 MCPData::SmallMessageSize,
                                                 &sMcpData->smallMessageCount,
                                                 1);

   sMcpData->largeMessagePool = IPCBufPoolCreate(sMcpData->largeMessageBuffer.data(),
                                                 static_cast<uint32_t>(sMcpData->largeMessageBuffer.size()),
                                                 MCPData::LargeMessageSize,
                                                 &sMcpData->largeMessageCount,
                                                 1);

   sMcpData->initialised = true;
}


void *
mcpAllocateMessage(uint32_t size)
{
   void *message = nullptr;

   if (size == 0) {
      return nullptr;
   } else if (size <= MCPData::SmallMessageSize) {
      message = IPCBufPoolAllocate(sMcpData->smallMessagePool, size);
   } else {
      message = IPCBufPoolAllocate(sMcpData->largeMessagePool, size);
   }

   std::memset(message, 0, size);
   return message;
}


MCPError
mcpFreeMessage(void *message)
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
                   MCPTitleListType *searchTitle,
                   MCPTitleListSearchFlags searchFlags,
                   MCPTitleListType *titleList,
                   uint32_t titleListLength)
{
   auto message = mcpAllocateMessage(sizeof(MCPRequestSearchTitleList));

   if (!message) {
      return static_cast<IOSError>(MCPError::Alloc);
   }

   auto request = reinterpret_cast<MCPRequestSearchTitleList *>(message);
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

} // namespace internal

void
Module::registerMcpFunctions()
{
   RegisterKernelFunction(MCP_Open);
   RegisterKernelFunction(MCP_Close);
   RegisterKernelFunction(MCP_GetOwnTitleInfo);
   RegisterKernelFunction(MCP_GetSysProdSettings);
   RegisterKernelFunction(MCP_GetTitleId);
   RegisterKernelFunction(MCP_GetTitleInfo);
   RegisterKernelFunction(MCP_TitleCount);
   RegisterKernelFunction(MCP_TitleList);
   RegisterKernelFunction(MCP_TitleListByAppType);
   RegisterKernelFunction(MCP_TitleListByUniqueId);
   RegisterKernelFunction(MCP_TitleListByUniqueIdAndIndexedDeviceAndAppType);
   RegisterInternalData(sMcpData);
}

} // namespace coreinit
