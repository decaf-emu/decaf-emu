#pragma once
#include "ios_kernel_messagequeue.h"
#include "ios/ios_ipc.h"

namespace ios::kernel
{

void
submitIpcRequest(phys_ptr<IpcRequest> request);

namespace internal
{

MessageQueueId
getIpcMessageQueueId();

} // namespace internal

} // namespace ios::kernel
