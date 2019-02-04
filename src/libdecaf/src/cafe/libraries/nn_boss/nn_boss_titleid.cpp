#include "nn_boss.h"
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_boss
{

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self)
{
   if (!self) {
      self = virt_cast<TitleID *>(ghs::malloc(sizeof(TitleID)));
      if (!self) {
         return nullptr;
      }
   }

   self->value = 0ull;
   return self;
}

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self,
                    virt_ptr<TitleID> other)
{
   if (!self) {
      self = virt_cast<TitleID *>(ghs::malloc(sizeof(TitleID)));
      if (!self) {
         return nullptr;
      }
   }

   self->value = other->value;
   return self;
}

virt_ptr<TitleID>
TitleID_Constructor(virt_ptr<TitleID> self,
                    uint64_t id)
{
   if (!self) {
      self = virt_cast<TitleID *>(ghs::malloc(sizeof(TitleID)));
      if (!self) {
         return nullptr;
      }
   }

   self->value = id;
   return self;
}

bool
TitleID_IsValid(virt_ptr<TitleID> self)
{
   return self->value != 0ull;
}

uint64_t
TitleID_GetValue(virt_ptr<TitleID> self)
{
   return self->value;
}

uint32_t
TitleID_GetTitleID(virt_ptr<TitleID> self)
{
   return static_cast<uint32_t>(self->value & 0xFFFFFFFF);
}

uint32_t
TitleID_GetTitleCode(virt_ptr<TitleID> self)
{
   return TitleID_GetTitleID(self);
}

uint32_t
TitleID_GetUniqueId(virt_ptr<TitleID> self)
{
   return (TitleID_GetTitleID(self) >> 8) & 0xFFFF;
}

bool
TitleID_OperatorEqual(virt_ptr<TitleID> self,
                      virt_ptr<TitleID> other)
{
   if (self->value & 0x2000000000ull) {
      return (self->value  & 0xFFFFFF00FFFFFFFFull) ==
             (other->value & 0xFFFFFF00FFFFFFFFull);
   }

   return self->value == other->value;
}

bool
TitleID_OperatorNotEqual(virt_ptr<TitleID> self,
                         virt_ptr<TitleID> other)
{
   return !TitleID_OperatorEqual(self, other);
}

void
Library::registerTitleIdSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss7TitleIDFv",
                              static_cast<virt_ptr<TitleID> (*)(virt_ptr<TitleID>)>(TitleID_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss7TitleIDFRCQ3_2nn4boss7TitleID",
                              static_cast<virt_ptr<TitleID>(*)(virt_ptr<TitleID>, virt_ptr<TitleID>)>(TitleID_Constructor));
   RegisterFunctionExportName("__ct__Q3_2nn4boss7TitleIDFUL",
                              static_cast<virt_ptr<TitleID>(*)(virt_ptr<TitleID>, uint64_t)>(TitleID_Constructor));

   RegisterFunctionExportName("IsValid__Q3_2nn4boss7TitleIDCFv",
                              TitleID_IsValid);
   RegisterFunctionExportName("GetValue__Q3_2nn4boss7TitleIDCFv",
                              TitleID_GetValue);
   RegisterFunctionExportName("GetTitleId__Q3_2nn4boss7TitleIDCFv",
                              TitleID_GetTitleID);
   RegisterFunctionExportName("GetTitleCode__Q3_2nn4boss7TitleIDCFv",
                              TitleID_GetTitleCode);
   RegisterFunctionExportName("GetUniqueId__Q3_2nn4boss7TitleIDCFv",
                              TitleID_GetUniqueId);
   RegisterFunctionExportName("__eq__Q3_2nn4boss7TitleIDCFRCQ3_2nn4boss7TitleID",
                              TitleID_OperatorEqual);
   RegisterFunctionExportName("__ne__Q3_2nn4boss7TitleIDCFRCQ3_2nn4boss7TitleID",
                              TitleID_OperatorNotEqual);
}

} // namespace cafe::nn_boss
