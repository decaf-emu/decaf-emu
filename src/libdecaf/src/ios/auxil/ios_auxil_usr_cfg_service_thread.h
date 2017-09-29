#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_messagequeue.h"
#include "ios_auxil_usr_cfg_device.h"

namespace ios::auxil::internal
{

using UCDeviceHandle = int32_t;

Error
startUsrCfgServiceThread();

kernel::MessageQueueId
getUsrCfgServiceMessageQueueId();

Error
getUCDevice(UCDeviceHandle handle,
            UCDevice **outDevice);

void
destroyUCDevice(UCDevice *device);

void
initialiseStaticUsrCfgServiceThreadData();

} // namespace ios::auxil::internal
