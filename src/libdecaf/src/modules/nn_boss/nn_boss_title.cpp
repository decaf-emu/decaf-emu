#include "nn_boss.h"
#include "nn_boss_title.h"
#include "nn_boss_result.h"
#include "modules/nn_act/nn_act_core.h"

namespace nn
{

namespace boss
{

ghs::VirtualTableEntry *
Title::VirtualTable = nullptr;

ghs::TypeDescriptor *
Title::TypeInfo = nullptr;

Title::Title()
{
   mVirtualTable = Title::VirtualTable;
}

Title::Title(uint32_t account, TitleID *title) :
   mAccountID(account),
   mTitleID(title)
{
}

Title::~Title()
{
}

nn::Result
Title::ChangeAccount(uint8_t slot)
{
   if (slot == 0) {
      mAccountID = 0;
   } else {
      auto id = nn::act::GetPersistentIdEx(slot);

      if (id == 0) {
         return nn::boss::InvalidParameter;
      }
   }

   return nn::boss::Success;
}

void
Module::registerTitleFunctions()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss5TitleFv", Title);
   RegisterKernelFunctionConstructorArgs("__ct__Q3_2nn4boss5TitleFUiQ3_2nn4boss7TitleID", Title, uint32_t, TitleID *);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn4boss5TitleFv", Title);

   RegisterKernelFunctionName("ChangeAccount__Q3_2nn4boss5TitleFUc", &Title::ChangeAccount);
}

void
Module::initialiseTitle()
{
   Title::TypeInfo = ghs::internal::makeTypeDescriptor("nn::boss::Title");

   Title::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, Title::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn4boss5TitleFv") },
   });
}

} // namespace boss

} // namespace nn
