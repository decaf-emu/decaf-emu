#pragma once
#include "nn_boss_enum.h"
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/act/nn_act_types.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::Storage::Storage(const char *, nn::boss::StorageKind)
nn::boss::Storage::Storage(const char *, unsigned int, nn::boss::StorageKind)
nn::boss::Storage::Storage(const nn::boss::Storage &)
nn::boss::Storage::Storage(unsigned char, const char *, nn::boss::StorageKind)
nn::boss::Storage::ClearAllHistories()
nn::boss::Storage::ClearHistory()
nn::boss::Storage::Exist() const
nn::boss::Storage::Finalize()
nn::boss::Storage::GetDataList(nn::boss::DataName *, unsigned int, unsigned int *, unsigned int) const
nn::boss::Storage::GetReadFlagFromNsDatas(const nn::boss::DataName *, unsigned int, bool *) const
nn::boss::Storage::GetUnreadDataList(nn::boss::DataName *, unsigned int, unsigned int *, unsigned int) const
nn::boss::Storage::Initialize(const char *, nn::boss::StorageKind)
nn::boss::Storage::Initialize(const char *, unsigned int, nn::boss::StorageKind)
nn::boss::Storage::Initialize(unsigned char, const char *, nn::boss::StorageKind)
nn::boss::Storage::SetReadFlagToNsDatas(const nn::boss::DataName *, unsigned int, bool *)
nn::boss::Storage::SetReadFlagToNsDatas(const nn::boss::DataName *, unsigned int, bool)
*/

namespace cafe::nn_boss
{

struct Storage
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<nn::act::PersistentId> persistentId;
   be2_val<StorageKind> storageKind;
   UNKNOWN(3);
   be2_array<char, 8> directoryName;
   UNKNOWN(5);
   be2_val<TitleID> titleID;
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
   UNKNOWN(4);
};
CHECK_OFFSET(Storage, 0x00, persistentId);
CHECK_OFFSET(Storage, 0x04, storageKind);
CHECK_OFFSET(Storage, 0x0B, directoryName);
CHECK_OFFSET(Storage, 0x18, titleID);
CHECK_OFFSET(Storage, 0x20, virtualTable);
CHECK_SIZE(Storage, 0x28);

virt_ptr<Storage>
Storage_Constructor(virt_ptr<Storage> self);

void
Storage_Destructor(virt_ptr<Storage> self,
                   ghs::DestructorFlags flags);

} // namespace cafe::nn_boss
