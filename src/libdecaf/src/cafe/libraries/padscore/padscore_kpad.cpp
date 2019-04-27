#include "padscore.h"
#include "padscore_kpad.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::padscore
{

void
KPADInit()
{
   decaf_warn_stub();
   KPADInitEx(NULL, 0);
}

void
KPADInitEx(virt_ptr<void> a1,
           uint32_t a2)
{
   decaf_warn_stub();
   WPADInit();
}

void
KPADShutdown()
{
   decaf_warn_stub();
}

//! Enable "Direct Pointing Device"
void
KPADEnableDPD(KPADChan chan)
{
   decaf_warn_stub();
}

//! Disable "Direct Pointing Device"
void
KPADDisableDPD(KPADChan chan)
{
   decaf_warn_stub();
}

uint32_t
KPADGetMplsWorkSize()
{
   return 0x5FE0;
}

void
KPADSetMplsWorkarea(virt_ptr<void> buffer)
{
   decaf_warn_stub();
}

int32_t
KPADRead(KPADChan chan,
         virt_ptr<KPADStatus> data,
         uint32_t size)
{
   decaf_warn_stub();
   auto readError = StackObject<KPADReadError> { };
   auto result = KPADReadEx(chan, data, size, readError);

   if (*readError != KPADReadError::OK) {
      return *readError;
   } else {
      return result;
   }
}

int32_t
KPADReadEx(KPADChan chan,
           virt_ptr<KPADStatus> data,
           uint32_t size,
           virt_ptr<KPADReadError> outError)
{
   decaf_warn_stub();

   if (outError) {
      *outError = KPADReadError::NoController;
   }

   return 0;
}

void
Library::registerKpadSymbols()
{
   RegisterFunctionExport(KPADInit);
   RegisterFunctionExport(KPADInitEx);
   RegisterFunctionExport(KPADShutdown);
   RegisterFunctionExport(KPADDisableDPD);
   RegisterFunctionExport(KPADEnableDPD);
   RegisterFunctionExport(KPADGetMplsWorkSize);
   RegisterFunctionExport(KPADSetMplsWorkarea);
   RegisterFunctionExport(KPADRead);
   RegisterFunctionExport(KPADReadEx);
}

} // namespace cafe::padscore
