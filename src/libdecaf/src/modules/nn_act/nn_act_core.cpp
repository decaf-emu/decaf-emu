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
GetUuid(UUID *uuid)
{
   return GetUuidEx(uuid, GetSlotNo());
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
      return nn::Result::Success;
   }

   return nn::act::AccountNotFound;
}

nn::Result
GetAccountId(char *accountId)
{
   return GetAccountIdEx(accountId, GetSlotNo());
}

nn::Result
GetAccountIdEx(char *accountId,
               uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *accountId = '\0';
   return nn::Result::Success;
}

uint8_t
GetParentalControlSlotNo()
{
   uint8_t parentSlot = 0;
   GetParentalControlSlotNoEx(&parentSlot, GetSlotNo());
   return parentSlot;
}

nn::Result
GetParentalControlSlotNoEx(uint8_t *parentSlot, uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *parentSlot = 1;
   return nn::Result::Success;
}

uint32_t
GetPersistentId()
{
   return GetPersistentIdEx(gCurrentSlot);
}

uint32_t
GetPersistentIdEx(uint8_t slot)
{
   if (slot == gCurrentSlot) {
      return 1;
   } else {
      return 0;
   }
}

uint32_t
GetPrincipalId()
{
   be_val<uint32_t> id;
   GetPrincipalIdEx(&id, GetSlotNo());
   return id;
}

nn::Result
GetPrincipalIdEx(be_val<uint32_t> *principalId,
                 uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *principalId = 1;
   return nn::Result::Success;
}

uint32_t
GetSimpleAddressId()
{
   be_val<uint32_t> id;
   GetSimpleAddressIdEx(&id, GetSlotNo());
   return id;
}

nn::Result
GetSimpleAddressIdEx(be_val<uint32_t> *simpleAddressId,
                     uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *simpleAddressId = 1;
   return nn::Result::Success;
}

uint64_t
GetTransferableId(uint32_t unk1)
{
   be_val<uint64_t> id;
   GetTransferableIdEx(&id, unk1, GetSlotNo());
   return id;
}

nn::Result
GetTransferableIdEx(be_val<uint64_t> *transferableId, uint32_t unk1, uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   *transferableId = 1;
   return nn::Result::Success;
}

nn::Result
GetMii(FFLStoreData *data)
{
   return GetMiiEx(data, GetSlotNo());
}

nn::Result
GetMiiEx(FFLStoreData *data, uint8_t slot)
{
   if (slot != gCurrentSlot) {
      return nn::act::AccountNotFound;
   }

   gLog->warn("GetMiiEx(0x{:08X}, {})", mem::untranslate(data), static_cast<uint32_t>(slot));
   return nn::Result::Success;
}

bool
IsParentalControlCheckEnabled()
{
   return false;
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

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn3actFv", nn::act::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn3actFv", nn::act::Finalize);
   RegisterKernelFunctionName("Cancel__Q2_2nn3actFv", nn::act::Cancel);
   RegisterKernelFunctionName("IsSlotOccupied__Q2_2nn3actFUc", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetSlotNo__Q2_2nn3actFv", nn::act::GetSlotNo);
   RegisterKernelFunctionName("GetMii__Q2_2nn3actFP12FFLStoreData", nn::act::GetMii);
   RegisterKernelFunctionName("GetMiiEx__Q2_2nn3actFP12FFLStoreDataUc", nn::act::GetMiiEx);
   RegisterKernelFunctionName("IsParentalControlCheckEnabled__Q2_2nn3actFv", nn::act::IsParentalControlCheckEnabled);
   RegisterKernelFunctionName("IsNetworkAccount__Q2_2nn3actFv", nn::act::IsNetworkAccount);
   RegisterKernelFunctionName("IsNetworkAccountEx__Q2_2nn3actFUc", nn::act::IsNetworkAccountEx);
   RegisterKernelFunctionName("GetNumOfAccounts__Q2_2nn3actFv", nn::act::GetNumOfAccounts);
   RegisterKernelFunctionName("GetUuid__Q2_2nn3actFP7ACTUuid", nn::act::GetUuid);
   RegisterKernelFunctionName("GetUuidEx__Q2_2nn3actFP7ACTUuidUc", nn::act::GetUuidEx);
   RegisterKernelFunctionName("GetAccountId__Q2_2nn3actFPc", nn::act::GetAccountId);
   RegisterKernelFunctionName("GetAccountIdEx__Q2_2nn3actFPcUc", nn::act::GetAccountIdEx);
   RegisterKernelFunctionName("GetParentalControlSlotNo__Q2_2nn3actFv", nn::act::GetParentalControlSlotNo);
   RegisterKernelFunctionName("GetParentalControlSlotNoEx__Q2_2nn3actFPUcUc", nn::act::GetParentalControlSlotNoEx);
   RegisterKernelFunctionName("GetPersistentId__Q2_2nn3actFv", nn::act::GetPersistentId);
   RegisterKernelFunctionName("GetPersistentIdEx__Q2_2nn3actFUc", nn::act::GetPersistentIdEx);
   RegisterKernelFunctionName("GetPrincipalId__Q2_2nn3actFv", nn::act::GetPrincipalId);
   RegisterKernelFunctionName("GetPrincipalIdEx__Q2_2nn3actFPUiUc", nn::act::GetPrincipalIdEx);
   RegisterKernelFunctionName("GetSimpleAddressId__Q2_2nn3actFv", nn::act::GetSimpleAddressId);
   RegisterKernelFunctionName("GetSimpleAddressIdEx__Q2_2nn3actFPUiUc", nn::act::GetSimpleAddressIdEx);
   RegisterKernelFunctionName("GetTransferableId__Q2_2nn3actFUi", nn::act::GetTransferableId);
   RegisterKernelFunctionName("GetTransferableIdEx__Q2_2nn3actFPULUiUc", nn::act::GetTransferableIdEx);
}

} // namespace act

} // namespace nn
