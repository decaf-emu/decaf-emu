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

FSAStatus
FSACloseFile(kernel::ResourceHandleId resourceHandleId,
             FSAFileHandle fileHandle);

FSAStatus
FSAReadFile(kernel::ResourceHandleId resourceHandleId,
            phys_ptr<void> buffer,
            uint32_t size,
            uint32_t count,
            FSAFileHandle fileHandle,
            FSAReadFlag readFlags);

FSAStatus
FSAWriteFile(kernel::ResourceHandleId resourceHandleId,
             phys_ptr<const void> buffer,
             uint32_t size,
             uint32_t count,
             FSAFileHandle fileHandle,
             FSAWriteFlag writeFlags);

// MakeDir
// StatFile
// Rename
// Remove

} // namespace ios::fs
