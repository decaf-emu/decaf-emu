#include "nn_act.h"
#include "nn_act_core.h"
#include "nn_act_result.h"

namespace nn
{

namespace act
{

nn::Result
Initialize()
{
   return nn::Result::Success;
}

void
Finalize()
{
}

bool
IsSlotOccupied(uint8_t id)
{
   return false;
}

nn::Result
Cancel()
{
   return nn::Result::Success;
}

uint8_t
GetSlotNo()
{
   return 0;
}

uint32_t
GetTransferableId(uint32_t unk1)
{
   return 0;
}

nn::Result GetMii(void* unk1)
{
   return nn::act::AccountNotFound;
}

nn::Result GetMiiEx(void* unk1, uint8_t unk2)
{
   gLog->warn("GetMiiEx({}, {})", reinterpret_cast<intptr_t>(unk1), unk2);
   return nn::act::AccountNotFound;
}

} // namespace act

} // namespace nn

void
NN_act::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3actFv", nn::act::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3actFv", nn::act::Finalize);
   RegisterKernelFunctionName("Cancel__Q2_2nn3actFv", nn::act::Cancel);
   RegisterKernelFunctionName("IsSlotOccupied__Q2_2nn3actFUc", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetSlotNo__Q2_2nn3actFv", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetTransferableId__Q2_2nn3actFUi", nn::act::GetTransferableId);
   RegisterKernelFunctionName("GetMii__Q2_2nn3actFP12FFLStoreData", nn::act::GetMii);
   RegisterKernelFunctionName("GetMiiEx__Q2_2nn3actFP12FFLStoreDataUc", nn::act::GetMiiEx);
}
