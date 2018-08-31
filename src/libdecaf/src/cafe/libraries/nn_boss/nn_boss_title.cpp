#include "nn_boss.h"
#include "nn_boss_result.h"
#include "nn_boss_title.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/nn_act/nn_act_lib.h"

namespace cafe::nn::boss
{

virt_ptr<hle::VirtualTable> Title::VirtualTable = nullptr;
virt_ptr<hle::TypeDescriptor> Title::TypeDescriptor = nullptr;

Title::Title() :
   mAccountID(0u),
   mTitleID(0ull),
   mVirtualTable(Title::VirtualTable)
{
}

Title::Title(uint32_t accountId,
             virt_ptr<TitleID> titleId) :
   mAccountID(accountId),
   mTitleID(titleId),
   mVirtualTable(Title::VirtualTable)
{
}

Title::~Title()
{
}

nn::Result
Title::ChangeAccount(uint8_t slot)
{
   if (!slot) {
      mAccountID = slot;
   } else if (auto accountId = nn::act::GetPersistentIdEx(slot)) {
      mAccountID = accountId;
   } else {
      return InvalidParameter;
   }

   return Success;
}

void
Library::registerTitleSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss5TitleFv",
                             Title);
   RegisterConstructorExportArgs("__ct__Q3_2nn4boss5TitleFUiQ3_2nn4boss7TitleID",
                                 Title, uint32_t, virt_ptr<TitleID>);
   RegisterDestructorExport("__dt__Q3_2nn4boss5TitleFv",
                            Title);

   RegisterFunctionExportName("ChangeAccount__Q3_2nn4boss5TitleFUc",
                              &Title::ChangeAccount);

   registerTypeInfo<Title>(
      "nn::boss::Title",
      {
         "__dt__Q3_2nn4boss5TitleFv",
      });
}

} // namespace cafe::nn::boss
