#pragma once
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_resourcemanager.h"
#include "ios/ios_result.h"
#include "ios/ios_ipc.h"

namespace ios::kernel
{

Result<ResourceHandleID>
IOS_Open(std::string_view device,
         OpenMode mode);

} // namespace ios::kernel
