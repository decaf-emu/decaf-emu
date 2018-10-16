#pragma once
#include "nn_boss_titleid.h"

#include "cafe/libraries/cafe_hle_library_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

class Title
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   Title();
   Title(uint32_t accountId,
         virt_ptr<TitleID> titleId);
   ~Title();

   nn::Result
   ChangeAccount(uint8_t slot);

private:
   be2_val<uint32_t> mAccountID;
   UNKNOWN(4);
   be2_val<TitleID> mTitleID;
   be2_virt_ptr<hle::VirtualTable> mVirtualTable;
   UNKNOWN(4);

protected:
   CHECK_MEMBER_OFFSET_BEG
      CHECK_OFFSET(Title, 0x00, mAccountID);
      CHECK_OFFSET(Title, 0x08, mTitleID);
      CHECK_OFFSET(Title, 0x10, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(Title, 0x18);

} // namespace cafe::nn_boss
