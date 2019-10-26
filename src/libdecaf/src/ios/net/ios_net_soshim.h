#pragma once
#include "ios/ios_enum.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

namespace ios::net
{

using SOShimHandle = kernel::ResourceHandleId;

Error
SOShim_Open();

Error
SOShim_Close(SOShimHandle handle);

Error
SOShim_GetProcessSocketHandle(SOShimHandle handle,
                           TitleId titleId,
                           ProcessId processId);

} // namespace ios::net
