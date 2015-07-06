#include "nn_act.h"
#include "nn_act_core.h"

namespace nn
{

namespace act
{

Result
Initialize()
{
   return Result::OK;
}

void
Finalize()
{
}

Result
Cancel()
{
   return Result::OK;
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

} // namespace act

} // namespace nn

void
NNAct::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3actFv", nn::act::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3actFv", nn::act::Finalize);
   RegisterKernelFunctionName("Cancel__Q2_2nn3actFv", nn::act::Cancel);
   RegisterKernelFunctionName("GetSlotNo__Q2_2nn3actFv", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetTransferableId__Q2_2nn3actFUi", nn::act::GetTransferableId);
}
