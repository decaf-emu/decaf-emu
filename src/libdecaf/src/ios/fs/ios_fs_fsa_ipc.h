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
FSAReadFileWithPos(FSAHandle handle,
                   phys_ptr<void> buffer,
                   uint32_t size,
                   uint32_t count,
                   uint32_t pos,
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

FSAStatus
FSARemove(FSAHandle handle,
          std::string_view name);

FSAStatus
FSAMakeDir(FSAHandle handle,
           std::string_view name,
           uint32_t mode);

FSAStatus
FSAMakeQuota(FSAHandle handle,
             std::string_view name,
             uint32_t mode,
             uint64_t quota);

FSAStatus
FSAMount(FSAHandle handle,
         std::string_view src,
         std::string_view dst,
         uint32_t unk0x500,
         phys_ptr<void> unkBuf,
         uint32_t unkBufLen);

FSAStatus
FSAGetInfoByQuery(FSAHandle handle,
                  std::string_view name,
                  FSAQueryInfoType query,
                  phys_ptr<void> output);

} // namespace ios::fs
