#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"

namespace ios::auxil::internal
{

Error
startUsrCfgThread();

kernel::MessageQueueId
getUsrCfgMessageQueueId();

} // namespace ios::auxil::internal
