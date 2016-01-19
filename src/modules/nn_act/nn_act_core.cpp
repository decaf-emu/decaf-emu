#include "nn_act.h"
#include "nn_act_core.h"
#include "nn_act_result.h"

static const uint8_t
gCurrentSlot = 1;

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

uint8_t
GetNumOfAccounts()
{
   return 1;
}

bool
IsSlotOccupied(uint8_t slot)
{
   if (slot == gCurrentSlot) {
      return true;
   } else {
      return false;
   }
}

nn::Result
Cancel()
{
   return nn::Result::Success;
}

uint8_t
GetSlotNo()
{
   return gCurrentSlot;
}

nn::Result
GetUuidEx(UUID *uuid,
          uint8_t slot)
{
   // System account
   if (slot == 255) {
      uuid->fill('X');
      uuid->at(0) = 's';
      uuid->at(1) = 'y';
      uuid->at(2) = 's';
      return nn::Result::Success;
   }

   // User account
   if (slot == gCurrentSlot) {
      uuid->fill('A' + slot);
      uuid->at(0) = 'u';
      uuid->at(1) = 's';
      uuid->at(2) = 'r';
      return nn::act::AccountNotFound;
   }

   return nn::act::AccountNotFound;
}

nn::Result
GetPrincipalIdEx(be_val<uint32_t> *principalId,
                 uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *principalId = 0;
   return nn::Result::Success;
}

nn::Result
GetSimpleAddressIdEx(be_val<uint32_t> *simpleAddressId,
                     uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *simpleAddressId = 0;
   return nn::Result::Success;
}

uint32_t
GetTransferableId(uint32_t unk1)
{
   return 0;
}

nn::Result
GetMii(void *data)
{
   gLog->warn("GetMii(0x{:08X})", memory_untranslate(data));
   return nn::act::AccountNotFound;
}

nn::Result
GetMiiEx(void *data, uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   gLog->warn("GetMiiEx(0x{:08X}, {})", memory_untranslate(data), static_cast<uint32_t>(slot));
   return nn::Result::Success;
}

bool
IsNetworkAccount()
{
   return false;
}

bool
IsNetworkAccountEx(uint8_t slot)
{
   return false;
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
   RegisterKernelFunctionName("IsNetworkAccount__Q2_2nn3actFv", nn::act::IsNetworkAccount);
   RegisterKernelFunctionName("IsNetworkAccountEx__Q2_2nn3actFUc", nn::act::IsNetworkAccountEx);
   RegisterKernelFunctionName("GetNumOfAccounts__Q2_2nn3actFv", nn::act::GetNumOfAccounts);
   RegisterKernelFunctionName("GetUuidEx__Q2_2nn3actFP7ACTUuidUc", nn::act::GetUuidEx);
   RegisterKernelFunctionName("GetPrincipalIdEx__Q2_2nn3actFPUiUc", nn::act::GetPrincipalIdEx);
   RegisterKernelFunctionName("GetSimpleAddressIdEx__Q2_2nn3actFPUiUc", nn::act::GetSimpleAddressIdEx);
}
