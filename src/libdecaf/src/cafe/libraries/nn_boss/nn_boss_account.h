#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"
#include "nn/boss/nn_boss_types.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

using PersistentId = nn::boss::PersistentId;

#pragma pack(push, 1)

struct Account
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> unk0x00;
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
};
CHECK_OFFSET(Account, 0x00, unk0x00);
CHECK_OFFSET(Account, 0x04, virtualTable);
CHECK_SIZE(Account, 0x8);

#pragma pack(pop)

virt_ptr<Account>
Account_Constructor(virt_ptr<Account> self,
                    uint32_t a1);

void
Account_Destructor(virt_ptr<Account> self,
                   ghs::DestructorFlags flags);

nn::Result
Account_AddAccount(PersistentId persistentId);

}  // namespace namespace cafe::nn_boss
