#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_sl
{

struct ITransferrer
{
   static virt_ptr<ghs::TypeIDStorage> TypeID;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_virt_ptr<ghs::VirtualTable> virtualTable;
};
CHECK_OFFSET(ITransferrer, 0x00, virtualTable);
CHECK_SIZE(ITransferrer, 0x4);

struct DrcTransferrer : public ITransferrer
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

};
CHECK_SIZE(DrcTransferrer, 0x4);

virt_ptr<DrcTransferrer>
DrcTransferrer_Constructor(virt_ptr<DrcTransferrer> self);

void
DrcTransferrer_Destructor(virt_ptr<DrcTransferrer> self,
                          ghs::DestructorFlags flags);

nn::Result
DrcTransferrer_Transfer1(virt_ptr<DrcTransferrer> self,
                         virt_ptr<void> arg1,
                         uint32_t arg2,
                         uint32_t arg3,
                         bool arg4);


nn::Result
DrcTransferrer_Transfer2(virt_ptr<DrcTransferrer> self,
                         virt_ptr<void> arg1,
                         uint32_t arg2,
                         uint32_t arg3,
                         uint32_t arg4);

nn::Result
DrcTransferrer_AbortTransfer(virt_ptr<DrcTransferrer> self);

nn::Result
DrcTransferrer_UnkFunc4(virt_ptr<DrcTransferrer> self);

}  // namespace cafe::nn_sl
