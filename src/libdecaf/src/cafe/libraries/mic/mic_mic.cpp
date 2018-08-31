#include "mic.h"
#include "mic_mic.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::mic
{

MICHandle
MICInit(uint32_t a1,
        virt_ptr<void> a2,
        virt_ptr<void> a3,
        virt_ptr<MICError> outError)
{
   decaf_warn_stub();

   if (outError) {
      *outError = -1;
   }

   return 0;
}

MICError
MICUninit(MICHandle handle)
{
   return -1;
}

MICError
MICOpen(MICHandle handle)
{
   return -1;
}

MICError
MICClose(MICHandle handle)
{
   return -1;
}

MICError
MICGetStatus(MICHandle handle,
             virt_ptr<MICStatus> status)
{
   return -1;
}

void
Library::registerMicSymbols()
{
   RegisterFunctionExport(MICInit);
   RegisterFunctionExport(MICUninit);
   RegisterFunctionExport(MICOpen);
   RegisterFunctionExport(MICClose);
   RegisterFunctionExport(MICGetStatus);
}

} // namespace cafe::mic
