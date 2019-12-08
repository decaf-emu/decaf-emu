#include "nn_boss.h"
#include "nn_boss_almightystorage.h"
#include "nn_boss_enum.h"
#include "nn_boss_storage.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/boss/nn_boss_result.h"

#include <common/strutils.h>

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> AlmightyStorage::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> AlmightyStorage::TypeDescriptor = nullptr;

virt_ptr<AlmightyStorage>
AlmightyStorage_Constructor(virt_ptr<AlmightyStorage> self)
{
   if (!self) {
      self = virt_cast<AlmightyStorage *>(ghs::malloc(sizeof(AlmightyStorage)));
      if (!self) {
         return nullptr;
      }
   }

   Storage_Constructor(virt_cast<Storage *>(self));
   self->virtualTable = AlmightyStorage::VirtualTable;
   return self;
}

void
AlmightyStorage_Destructor(virt_ptr<AlmightyStorage> self,
                           ghs::DestructorFlags flags)
{
   Storage_Destructor(virt_cast<Storage *>(self), ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
AlmightyStorage_Initialize(virt_ptr<AlmightyStorage> self,
                           virt_ptr<TitleID> titleId,
                           virt_ptr<const char> directory,
                           nn::act::PersistentId persistentId,
                           StorageKind storageKind)
{
   if (!directory) {
      return ResultInvalidParameter;
   }

   self->titleID = *titleId;
   self->persistentId = persistentId;
   self->storageKind = storageKind;
   self->directoryName = directory.get();
   self->directoryName[7] = '\0';
   return ResultSuccess;
}

void
Library::registerAlmightyStorageSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss15AlmightyStorageFv",
                             static_cast<virt_ptr<AlmightyStorage> (*)(virt_ptr<AlmightyStorage>)>(AlmightyStorage_Constructor));
   RegisterFunctionExportName("__dt__Q3_2nn4boss15AlmightyStorageFv",
                              AlmightyStorage_Destructor);
   RegisterFunctionExportName("Initialize__Q3_2nn4boss15AlmightyStorageFQ3_2nn4boss7TitleIDPCcUiQ3_2nn4boss11StorageKind",
                              AlmightyStorage_Initialize);

   RegisterTypeInfo(
      AlmightyStorage,
      "nn::boss::AlmightyStorage",
      {
         "__dt__Q3_2nn4boss15AlmightyStorageFv",
      },
      {
         "nn::boss::Storage",
      });
}

} // namespace cafe::nn_boss
