#pragma once
#include "coreinit_enum.h"
#include "coreinit_fsa.h"

namespace cafe::coreinit
{

FSAStatus
FSACloseFile(FSAClientHandle clientHandle,
             FSAFileHandle fileHandle);

FSAStatus
FSAGetStat(FSAClientHandle clientHandle,
           virt_ptr<const char> path,
           virt_ptr<FSAStat> stat);

FSAStatus
FSAMakeDir(FSAClientHandle clientHandle,
           virt_ptr<const char> path,
           uint32_t permissions);

FSAStatus
FSAMount(FSAClientHandle clientHandle,
         virt_ptr<const char> path,
         virt_ptr<const char> target,
         uint32_t unk0,
         virt_ptr<void> unkBuf,
         uint32_t unkBufLen);

FSAStatus
FSAOpenFile(FSAClientHandle clientHandle,
            virt_ptr<const char> path,
            virt_ptr<const char> mode,
            virt_ptr<FSAFileHandle> outHandle);

FSAStatus
FSAReadFile(FSAClientHandle clientHandle,
            virt_ptr<uint8_t> buffer,
            uint32_t size,
            uint32_t count,
            FSAFileHandle fileHandle,
            FSAReadFlag readFlags);

FSAStatus
FSARemove(FSAClientHandle clientHandle,
          virt_ptr<const char> path);

FSAStatus
FSAWriteFile(FSAClientHandle clientHandle,
             virt_ptr<const uint8_t> buffer,
             uint32_t size,
             uint32_t count,
             FSAFileHandle fileHandle,
             FSAWriteFlag writeFlags);

} // namespace cafe::coreinit
