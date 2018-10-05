#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::snduser2
{

struct SPSoundEntry
{
   UNKNOWN(0x1C);
};
CHECK_SIZE(SPSoundEntry, 0x1C);

struct SPSoundTable
{
   be2_val<uint32_t> numEntries;

   //! This is actually a dynamically sized array.
   be2_array<SPSoundEntry, 1> entries;
};
CHECK_OFFSET(SPSoundTable, 0x00, numEntries);
CHECK_OFFSET(SPSoundTable, 0x04, entries);

void
SPInitSoundTable(virt_ptr<SPSoundTable> table,
                 virt_ptr<int32_t> samples,
                 virt_ptr<uint32_t> outSamplesSize);

virt_ptr<SPSoundEntry>
SPGetSoundEntry(virt_ptr<SPSoundTable> table,
                uint32_t index);

} // namespace cafe::snduser2
