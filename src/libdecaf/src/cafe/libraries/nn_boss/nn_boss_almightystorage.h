#pragma once
#include "nn_boss_storage.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::AlmightyStorage::GetReadFlagFromNsDatas(const nn::boss::DataName *, unsigned int, bool *) const
nn::boss::AlmightyStorage::GetBossStorageDirectoryList(unsigned int, nn::boss::TitleID, nn::boss::StorageKind, nn::boss::DataName *, unsigned int, unsigned int *, unsigned int)
nn::boss::AlmightyStorage::SetReadFlagToNsDatas(const nn::boss::DataName *, unsigned int, bool *)
nn::boss::AlmightyStorage::SetReadFlagToNsDatas(const nn::boss::DataName *, unsigned int, bool)
*/

namespace cafe::nn_boss
{

struct AlmightyStorage : public Storage
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(AlmightyStorage, 0x28);

virt_ptr<AlmightyStorage>
AlmightyStorage_Constructor(virt_ptr<AlmightyStorage> self);

void
AlmightyStorage_Destructor(virt_ptr<AlmightyStorage> self,
                           ghs::DestructorFlags flags);

nn::Result
AlmightyStorage_Initialize(virt_ptr<AlmightyStorage> self,
                           virt_ptr<TitleID> titleId,
                           virt_ptr<const char> directory,
                           nn::act::PersistentId persistentId,
                           StorageKind storageKind);

} // namespace cafe::nn_boss
