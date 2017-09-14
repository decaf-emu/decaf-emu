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

using IpcRequest = ios::IpcRequest;

void
ipcDriverKernelSubmitRequest(IpcRequest *request);

void
ipcDriverKernelSubmitReply(IpcRequest *reply);

void
ipcDriverKernelHandleInterrupt();

/** @} */

} // namespace kernel
