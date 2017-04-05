#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mcp.h"

#include <common/decaf_assert.h>

namespace coreinit
{

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
   auto input = internal::mcpAllocateMessage(sizeof(uint32_t));

   if (!input) {
      result = MCPError::AllocError;
      goto out;
   }

   auto output = internal::mcpAllocateMessage(sizeof(MCPTitleListType));

   if (!output) {
      result = MCPError::AllocError;
      goto out;
   }

   // TODO: __KernelGetInfo(0, &in32, 0xA8, 0);
   auto in32 = reinterpret_cast<be_val<uint32_t> *>(input);
   *in32 = 0;

   auto iosError = IOS_Ioctl(handle,
                             MCPCommand::GetOwnTitleInfo,
                             input,
                             sizeof(uint32_t),
                             output,
                             sizeof(MCPTitleListType));

   result = internal::mcpDecodeIosErrorToMcpError(iosError);

   if (result >= 0) {
      std::memcpy(titleInfo, output, sizeof(MCPTitleListType));
   }

out:
   if (input) {
      internal::mcpFreeMessage(input);
      input = nullptr;
   }

   if (output) {
      internal::mcpFreeMessage(output);
      output = nullptr;
   }

   return result;
}


MCPError
MCP_GetSysProdSettings(IOSHandle handle,
                       MCPSysProdSettings *settings)
{
   if (!settings) {
      return MCPError::InvalidArg;
   }

   auto message = internal::mcpAllocateMessage(sizeof(IOSVec));

   if (!message) {
      return MCPError::AllocError;
   }

   auto outVec = reinterpret_cast<IOSVec *>(message);
   outVec->paddr = settings;
   outVec->len = sizeof(MCPSysProdSettings);

   auto iosError = IOS_Ioctlv(handle, MCPCommand::GetSysProdSettings, 0, 1, outVec);
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
      result = MCPError::AllocError;
      goto out;
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

out:
   if (output) {
      internal::mcpFreeMessage(output);
      output = nullptr;
   }

   return result;
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

   return MCPError::InvalidOp;
}

MCPError
mcpDecodeIosErrorToMcpError(IOSError error)
{
   auto category = ((~error) >> 16) & 0x3FF;

   if (error >= 0 || category == 0x4) {
      return static_cast<MCPError>(error);
   } else {
      return static_cast<MCPError>(0xFFFF0000 | error);
   }
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
   RegisterInternalData(sMcpData);
}

} // namespace coreinit
