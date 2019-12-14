#include "nn_boss.h"
#include "nn_boss_storage.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> Storage::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> Storage::TypeDescriptor = nullptr;

virt_ptr<Storage>
Storage_Constructor(virt_ptr<Storage> self)
{
   if (!self) {
      self = virt_cast<Storage *>(ghs::malloc(sizeof(Storage)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = Storage::VirtualTable;
   TitleID_Constructor(virt_addrof(self->titleID), 0ull);
   return self;
}

void
Storage_Destructor(virt_ptr<Storage> self,
                   ghs::DestructorFlags flags)
{
   self->virtualTable = Storage::VirtualTable;
   // TODO: Call Storage_Finalize(self);
   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

void
Library::registerStorageSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss7StorageFv",
                             static_cast<virt_ptr<Storage> (*)(virt_ptr<Storage>)>(Storage_Constructor));
   RegisterFunctionExportName("__dt__Q3_2nn4boss7StorageFv",
                              Storage_Destructor);

   RegisterTypeInfo(
      Storage,
      "nn::boss::Storage",
      {
         "__dt__Q3_2nn4boss7StorageFv",
      },
      {});
}

} // namespace cafe::nn_boss
