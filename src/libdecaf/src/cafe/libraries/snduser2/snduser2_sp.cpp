#include "snduser2.h"
#include "snduser2_sp.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

void
SPInitSoundTable(virt_ptr<SPSoundTable> table,
                 virt_ptr<int32_t> samples,
                 virt_ptr<uint32_t> outSamplesSize)
{
   decaf_warn_stub();
}

virt_ptr<SPSoundEntry>
SPGetSoundEntry(virt_ptr<SPSoundTable> table,
                uint32_t index)
{
   if (index >= table->numEntries) {
      return nullptr;
   }

   return virt_addrof(table->entries) + index;
}

void
Library::registerSpSymbols()
{
   RegisterFunctionExport(SPInitSoundTable);
   RegisterFunctionExport(SPGetSoundEntry);
   //RegisterFunctionExport(SPPrepareSound);
   //RegisterFunctionExport(SPPrepareEnd);
}

} // namespace cafe::snduser2
