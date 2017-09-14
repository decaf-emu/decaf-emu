#pragma once
#include "ios_kernel_messagequeue.h"
#include "ios/ios_ipc.h"

namespace ios::kernel
{

void
submitIpcRequest(phys_ptr<IpcRequest> request);

namespace internal
{

MessageQueueID
getIpcMessageQueueID();

} // namespace internal

} // namespace ios::kernel
