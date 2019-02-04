#pragma once
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct Title
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> accountID;
   UNKNOWN(4);
   be2_val<TitleID> titleID;
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
   UNKNOWN(4);
};
CHECK_OFFSET(Title, 0x00, accountID);
CHECK_OFFSET(Title, 0x08, titleID);
CHECK_OFFSET(Title, 0x10, virtualTable);
CHECK_SIZE(Title, 0x18);

virt_ptr<Title>
Title_Constructor(virt_ptr<Title> self);

virt_ptr<Title>
Title_Constructor(virt_ptr<Title> self,
                  uint32_t accountId,
                  virt_ptr<TitleID> titleId);

void
Title_Destructor(virt_ptr<Title> self,
                 ghs::DestructorFlags flags);

nn::Result
Title_ChangeAccount(virt_ptr<Title> self,
                    uint8_t slot);

} // namespace cafe::nn_boss
