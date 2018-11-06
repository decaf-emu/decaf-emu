#pragma once
#include <libcpu/be2_struct.h>

namespace nn
{

enum class TlsEntryEvent
{
   GetSize     = 0,
   Create      = 1,
   Destroy     = 2,
   Copy        = 3,
};

using TlsEntryEventFn = uint32_t(*)(TlsEntryEvent event,
                                    phys_ptr<void> dst,
                                    phys_ptr<void> copySrc);

struct TlsEntry
{
   uint32_t dataOffset;
   TlsEntry *next;
   TlsEntryEventFn eventFn = nullptr;
   bool supportsCopy;
};
/*
CHECK_OFFSET(TlsEntry, 0x0, dataOffset);
CHECK_OFFSET(TlsEntry, 0x4, next);
CHECK_OFFSET(TlsEntry, 0x8, entryEventFn);
CHECK_OFFSET(TlsEntry, 0xC, unk0x0c); // I think this is maybe a can copy flag
CHECK_SIZE(TlsEntry, 0x10);
*/

void
tlsAllocateEntry(TlsEntry &entry,
                 TlsEntryEventFn eventCallback,
                 bool supportsCopy = false);

uint32_t
tlsGetDataSize();

void
tlsInitialiseData(phys_ptr<void> data,
                  phys_ptr<void> copySrc = nullptr);

void
tlsDestroyData(phys_ptr<void> data);

void
tlsInitialiseThread(phys_ptr<void> data);

phys_ptr<void>
tlsGetEntry(TlsEntry &entry);

} // namespace nn
