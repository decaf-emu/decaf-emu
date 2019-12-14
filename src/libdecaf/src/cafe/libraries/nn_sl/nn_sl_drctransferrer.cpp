#include "nn_sl.h"
#include "nn_sl_drctransferrer.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_sl
{

virt_ptr<ghs::TypeIDStorage> ITransferrer::TypeID = nullptr;
virt_ptr<ghs::TypeDescriptor> ITransferrer::TypeDescriptor = nullptr;

virt_ptr<ghs::VirtualTable> DrcTransferrer::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> DrcTransferrer::TypeDescriptor = nullptr;

virt_ptr<DrcTransferrer>
DrcTransferrer_Constructor(virt_ptr<DrcTransferrer> self)
{
   if (!self) {
      self = virt_cast<DrcTransferrer *>(ghs::malloc(sizeof(DrcTransferrer)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = DrcTransferrer::VirtualTable;
   return self;
}

void
DrcTransferrer_Destructor(virt_ptr<DrcTransferrer> self,
                          ghs::DestructorFlags flags)
{
   if (self && (flags & ghs::DestructorFlags::FreeMemory)) {
      ghs::free(self);
   }
}

nn::Result
DrcTransferrer_Transfer1(virt_ptr<DrcTransferrer> self,
                         virt_ptr<void> arg1,
                         uint32_t arg2,
                         uint32_t arg3,
                         bool arg4)
{
   return DrcTransferrer_Transfer2(self, arg1, arg2, arg3, arg4 ? 3 : 1);
}

nn::Result
DrcTransferrer_Transfer2(virt_ptr<DrcTransferrer> self,
                         virt_ptr<void> arg1,
                         uint32_t arg2,
                         uint32_t arg3,
                         uint32_t arg4)
{
   decaf_warn_stub();
   return { 0xA183ED80 };
}

nn::Result
DrcTransferrer_AbortTransfer(virt_ptr<DrcTransferrer> self)
{
   decaf_warn_stub();
   return { 0xA183ED80 };
}

nn::Result
DrcTransferrer_UnkFunc4(virt_ptr<DrcTransferrer> self)
{
   // Not a stub, this is actual implementation!
   return { 0xA183E800 };
}

nn::Result
DrcTransferrer_DisplayNotification(virt_ptr<DrcTransferrer> self,
                                   uint32_t arg2,
                                   uint32_t arg3,
                                   uint32_t arg4)
{
   decaf_warn_stub();
   return { 0xA183E800 };
}

void
Library::registerDrcTransferrerSymbols()
{
   RegisterFunctionInternalName("__dt__Q5_2nn2sl4core6detail14DrcTransferrerFv",
                                DrcTransferrer_Destructor);
   RegisterFunctionInternalName("DrcTransferrer_Transfer1",
                                DrcTransferrer_Transfer1);
   RegisterFunctionInternalName("DrcTransferrer_Transfer2",
                                DrcTransferrer_Transfer2);
   RegisterFunctionInternalName("DrcTransferrer_AbortTransfer",
                                DrcTransferrer_AbortTransfer);
   RegisterFunctionInternalName("DrcTransferrer_UnkFunc4",
                                DrcTransferrer_UnkFunc4);
   RegisterFunctionInternalName("DrcTransferrer_DisplayNotification",
                                DrcTransferrer_DisplayNotification);
   RegisterDataExportName("__TID_Q3_2nn2sl12ITransferrer", ITransferrer::TypeID);

   RegisterTypeInfo(
      ITransferrer,
      "nn::sl::ITransferrer",
      {},
      {},
      "__TID_Q3_2nn2sl12ITransferrer");

   RegisterTypeInfo(
      DrcTransferrer,
      "nn::sl::core::detail::DrcTransferrer",
      {
         "__dt__Q5_2nn2sl4core6detail14DrcTransferrerFv",
         "DrcTransferrer_Transfer1",
         "DrcTransferrer_Transfer2",
         "DrcTransferrer_AbortTransfer",
         "DrcTransferrer_UnkFunc4",
         "DrcTransferrer_DisplayNotification",
      },
      {
         "nn::sl::ITransferrer",
      });
}

}  // namespace cafe::nn_sl
