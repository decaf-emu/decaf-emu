#pragma once
#include "ios_fs_enum.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/ios_enum.h"

namespace ios::fs::internal
{

Error
startFsaAsyncTaskThread();

void
initialiseStaticFsaAsyncTaskData();

void
fsaAsyncTaskComplete(phys_ptr<ios::kernel::ResourceRequest> resourceRequest,
                     FSAStatus result);

} // namespace ios::fs::internal
