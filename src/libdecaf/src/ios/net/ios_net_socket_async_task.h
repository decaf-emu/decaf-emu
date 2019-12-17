#pragma once
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/ios_enum.h"

#include <optional>

namespace ios::net::internal
{

Error
startSocketAsyncTaskThread();

void
completeSocketTask(phys_ptr<kernel::ResourceRequest> resourceRequest,
                   std::optional<Error> result);

void
initialiseStaticSocketAsyncTaskData();

} // namespace ios::net::internal
