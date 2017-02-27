#pragma once
#include "modules/coreinit/coreinit_ghs_typeinfo.h"
#include "modules/nn_result.h"
#include "nn_boss_titleid.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

/*
Unimplemented functions:
nn::boss::Title::GetNewArrivalFlag(void) const
nn::boss::Title::GetOptoutFlag(void) const
nn::boss::Title::SetNewArrivalFlagOff(void)
nn::boss::Title::SetOptoutFlag(bool)
*/

namespace nn
{

namespace boss
{

class Title
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   Title();
   Title(uint32_t account, TitleID *title);
   ~Title();

   nn::Result
   ChangeAccount(uint8_t slot);

private:
   uint32_t mAccountID;
   UNKNOWN(4);
   TitleID mTitleID;
   be_ptr<ghs::VirtualTableEntry> mVirtualTable;
   UNKNOWN(4);

protected:
   CHECK_MEMBER_OFFSET_START
      CHECK_OFFSET(Title, 0x00, mAccountID);
      CHECK_OFFSET(Title, 0x08, mTitleID);
      CHECK_OFFSET(Title, 0x10, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(Title, 0x18);

} // namespace boss

} // namespace nn
