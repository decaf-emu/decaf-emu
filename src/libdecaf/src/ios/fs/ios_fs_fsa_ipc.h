#pragma once
#include "ios_fs_fsa_types.h"
#include "ios/kernel/ios_kernel_ipc.h"

namespace ios::fs
{

using FSAHandle = kernel::ResourceHandleId;

Error
FSAOpen();

Error
FSAClose(FSAHandle handle);

FSAStatus
FSAOpenFile(FSAHandle handle,
            std::string_view name,
            std::string_view mode,
            FSAFileHandle *outHandle);

FSAStatus
FSACloseFile(FSAHandle handle,
             FSAFileHandle fileHandle);

FSAStatus
FSAReadFile(FSAHandle handle,
            phys_ptr<void> buffer,
            uint32_t size,
            uint32_t count,
            FSAFileHandle fileHandle,
            FSAReadFlag readFlags);

FSAStatus
FSAWriteFile(FSAHandle handle,
             phys_ptr<const void> buffer,
             uint32_t size,
             uint32_t count,
             FSAFileHandle fileHandle,
             FSAWriteFlag writeFlags);

FSAStatus
FSAStatFile(FSAHandle handle,
            FSAFileHandle fileHandle,
            phys_ptr<FSAStat> stat);

// MakeDir
// Rename
// Remove

} // namespace ios::fs
