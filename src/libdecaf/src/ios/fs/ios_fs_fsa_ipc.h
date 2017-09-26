#pragma once
#include "ios_fs_fsa_types.h"
#include "ios/kernel/ios_kernel_ipc.h"

namespace ios::fs
{

FSAStatus
FSAOpenFile(kernel::ResourceHandleId resourceHandleId,
            std::string_view name,
            std::string_view mode,
            FSAFileHandle *outHandle);

} // namespace ios::fs
