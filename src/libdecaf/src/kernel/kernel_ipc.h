#pragma once
#include "kernel_enum.h"
#include "ios/ios_ipc.h"

#include <array>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace kernel
{

/**
 * \defgroup kernel_ipc IPC
 * \ingroup kernel
 * @{
 */

struct IpcRequest
{
   //! Actual IPC request
   be2_struct<ios::IpcRequest> request;

   //! Allegedly the previous IPC command
   be2_val<ios::Command> prevCommand;

   //! Allegedly the previous IPC handle
   be2_val<int32_t> prevHandle;

   //! Buffer argument 1
   be2_virt_ptr<void> buffer1;

   //! Buffer argument 2
   be2_virt_ptr<void> buffer2;

   //! Buffer to copy device name to for IOS_Open
   std::array<char, 0x20> nameBuffer;

   UNKNOWN(0x80 - 0x68);
};
CHECK_OFFSET(IpcRequest, 0x00, request);
CHECK_OFFSET(IpcRequest, 0x38, prevCommand);
CHECK_OFFSET(IpcRequest, 0x3C, prevHandle);
CHECK_OFFSET(IpcRequest, 0x40, buffer1);
CHECK_OFFSET(IpcRequest, 0x44, buffer2);
CHECK_OFFSET(IpcRequest, 0x48, nameBuffer);
CHECK_SIZE(IpcRequest, 0x80);

ios::Error
ipcDriverKernelSubmitRequest(virt_ptr<IpcRequest> request);

void
ipcDriverKernelSubmitReply(phys_ptr<ios::IpcRequest> reply);

void
ipcDriverKernelHandleInterrupt();

/** @} */

} // namespace kernel
