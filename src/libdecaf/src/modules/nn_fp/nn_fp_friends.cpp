#include "nn_fp.h"
#include "nn_fp_friends.h"

namespace nn
{

namespace fp
{

nn::Result
GetFriendList(void *list,
              be_val<uint32_t> *length,
              uint32_t index,
              uint32_t listSize)
{
   *length = 0;
   return nn::Result::Success;
}

void
Module::registerFriendsFunctions()
{
   RegisterKernelFunctionName("GetFriendList__Q2_2nn2fpFPUiT1UiT3", nn::fp::GetFriendList);
}

} // namespace fp

} // namespace nn
