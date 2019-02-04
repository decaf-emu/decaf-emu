#include "nn_boss.h"
#include "nn_boss_title.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "cafe/libraries/nn_act/nn_act_lib.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> Title::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> Title::TypeDescriptor = nullptr;

virt_ptr<Title>
Title_Constructor(virt_ptr<Title> self)
{
   if (!self) {
      self = virt_cast<Title *>(ghs::malloc(sizeof(Title)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = Title::VirtualTable;
   self->accountID = 0u;
   TitleID_Constructor(virt_addrof(self->titleID), 0ull);
   return self;
}

virt_ptr<Title>
Title_Constructor(virt_ptr<Title> self,
                  uint32_t accountId,
                  virt_ptr<TitleID> titleId)
{
   if (!self) {
      self = virt_cast<Title *>(ghs::malloc(sizeof(Title)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = Title::VirtualTable;
   self->accountID = accountId;
   TitleID_Constructor(virt_addrof(self->titleID), titleId);
   return self;
}

void
Title_Destructor(virt_ptr<Title> self,
                 ghs::DestructorFlags flags)
{
   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
Title_ChangeAccount(virt_ptr<Title> self,
                    uint8_t slot)
{
   if (!slot) {
      self->accountID = slot;
   } else if (auto accountId = nn_act::GetPersistentIdEx(slot)) {
      self->accountID = accountId;
   } else {
      return ResultInvalidParameter;
   }

   return ResultSuccess;
}

void
Library::registerTitleSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss5TitleFv",
                             static_cast<virt_ptr<Title> (*)(virt_ptr<Title>)>(Title_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss5TitleFUiQ3_2nn4boss7TitleID",
                              static_cast<virt_ptr<Title>(*)(virt_ptr<Title>, uint32_t, virt_ptr<TitleID>)>(Title_Constructor));
   RegisterFunctionExportName("__dt__Q3_2nn4boss5TitleFv",
                              Title_Destructor);

   RegisterFunctionExportName("ChangeAccount__Q3_2nn4boss5TitleFUc",
                              Title_ChangeAccount);

   registerTypeInfo<Title>(
      "nn::boss::Title",
      {
         "__dt__Q3_2nn4boss5TitleFv",
      });
}

} // namespace cafe::nn_boss
