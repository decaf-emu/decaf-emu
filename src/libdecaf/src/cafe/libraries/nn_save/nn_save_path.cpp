#include "nn_save.h"
#include "nn_save_path.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_fs_client.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"
#include "cafe/libraries/nn_act/nn_act_lib.h"
#include "cafe/libraries/nn_acp/nn_acp_client.h"
#include "cafe/libraries/nn_acp/nn_acp_saveservice.h"

#include <fmt/format.h>

using namespace cafe::coreinit;
using namespace nn::act;
using namespace nn::acp;

namespace cafe::nn_save
{

struct StaticSaveData
{
   be2_val<bool> initialised;
   be2_struct<OSMutex> mutex;
   be2_struct<FSClient> fsClient;
   be2_struct<FSCmdBlock> fsCmdBlock;
   be2_array<uint32_t, MaxSlot> persistentIdCache;
};

static virt_ptr<StaticSaveData> sSaveData = nullptr;

SaveStatus
SAVEInit()
{
   if (sSaveData->initialised) {
      return SaveStatus::OK;
   }

   OSInitMutex(virt_addrof(sSaveData->mutex));

   nn_act::Initialize();
   nn_acp::ACPInitialize();
   FSAddClient(virt_addrof(sSaveData->fsClient), FSErrorFlag::None);
   FSInitCmdBlock(virt_addrof(sSaveData->fsCmdBlock));

   for (auto i = SlotNo { 1 }; i <= MaxSlot; ++i) {
      sSaveData->persistentIdCache[i - 1] = nn_act::GetPersistentIdEx(i);
   }

   // Mount external storage if it is required
   if (OSGetUPID() == kernel::UniqueProcessId::Game) {
      StackObject<int32_t> externalStorageRequired;
      if (nn_acp::ACPIsExternalStorageRequired(externalStorageRequired).ok() &&
          *externalStorageRequired) {
         nn_acp::ACPMountExternalStorage();
      }
   }

   nn_acp::ACPMountSaveDir();
   nn_acp::ACPRepairSaveMetaDir();
   sSaveData->initialised = true;
   return SaveStatus::OK;
}

void
SAVEShutdown()
{
   if (sSaveData->initialised) {
      FSDelClient(virt_addrof(sSaveData->fsClient), FSErrorFlag::None);
      nn_acp::ACPFinalize();
      nn_act::Finalize();
      sSaveData->initialised = false;
   }
}

SaveStatus
SAVEInitSaveDir(uint8_t slotNo)
{
   if (!sSaveData->initialised) {
      coreinit::internal::OSPanic("save.cpp", 630,
         "SAVE library is not initialized. Call SAVEInit() prior to this function.\n");
   }

   auto persistentId = uint32_t { 0 };
   if (!internal::getPersistentId(slotNo, persistentId)) {
      return SaveStatus::NotFound;
   }

   auto result = nn_acp::ACPCreateSaveDir(persistentId,
                                          ACPDeviceType::Unknown1);
   // TODO: Update ACPGetApplicationBox
   return internal::translateResult(result);
}


SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID,
                           virt_ptr<const char> dir,
                           virt_ptr<char> buffer,
                           uint32_t bufferSize)
{
   if (!sSaveData->initialised) {
      coreinit::internal::OSPanic("save.cpp", 2543,
                                  "SAVE library is not initialized. Call SAVEInit() prior to this function.\n");
   }

   StackObject<int32_t> externalStorageRequired;
   auto storage = "storage_mlc01";

   if (nn_acp::ACPIsExternalStorageRequired(externalStorageRequired).ok()) {
      if (*externalStorageRequired) {
         storage = "storage_hfiomlc01";
      }
   } else {
      return SaveStatus::FatalError;
   }

   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = std::snprintf(buffer.get(), bufferSize,
                               "/vol/%s/sys/title/%08x/%08x/content/%s",
                               storage, titleHi, titleLo, dir.get());

   if (result < 0 || static_cast<uint32_t>(result) >= bufferSize) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


SaveStatus
SAVEGetSharedSaveDataPath(uint64_t titleID,
                          virt_ptr<const char> dir,
                          virt_ptr<char> buffer,
                          uint32_t bufferSize)
{
   if (!sSaveData->initialised) {
      coreinit::internal::OSPanic("save.cpp", 2543,
                                  "SAVE library is not initialized. Call SAVEInit() prior to this function.\n");
   }

   StackObject<int32_t> externalStorageRequired;
   auto storage = "storage_mlc01";

   if (nn_acp::ACPIsExternalStorageRequired(externalStorageRequired).ok()) {
      if (*externalStorageRequired) {
         storage = "storage_hfiomlc01";
      }
   } else {
      return SaveStatus::FatalError;
   }

   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = std::snprintf(buffer.get(), bufferSize,
                               "/vol/%s/usr/save/%08x/%08x/content/%s",
                               storage, titleHi, titleLo, dir.get());

   if (result < 0 || static_cast<uint32_t>(result) >= bufferSize) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


namespace internal
{

SaveStatus
translateResult(nn::Result result)
{
   if (result.ok()) {
      return SaveStatus::OK;
   }

   if (result.code == 0xFFFFFE0C) {
      return SaveStatus::NotFound;
   } else if (result.code == 0xFFFFFA23) {
      return SaveStatus::StorageFull;
   }

   return SaveStatus::MediaError;
}

bool
getPersistentId(SlotNo slot,
                uint32_t &outPersistentId)
{
   if (slot == SystemSlot) {
      outPersistentId = 0;
      return true;
   } else if (slot >= 1 && slot <= MaxSlot) {
      outPersistentId = sSaveData->persistentIdCache[slot - 1];
      return true;
   }

   return false;
}

vfs::Path
getSaveDirectory(SlotNo slot)
{
   auto persistentId = uint32_t { 0 };
   getPersistentId(slot, persistentId);

   if (persistentId == 0) {
      return fmt::format("/vol/save/common");
   } else {
      return fmt::format("/vol/save/{:08X}", persistentId);
   }
}

vfs::Path
getSavePath(SlotNo slot,
            std::string_view path)
{
   return getSaveDirectory(slot) / path;
}

vfs::Path
getTitleSaveDirectory(uint64_t title,
                      SlotNo slot)
{
   auto titleLo = static_cast<uint32_t>(title & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(title >> 32);
   auto persistentId = uint32_t { 0 };
   getPersistentId(slot, persistentId);

   if (persistentId == 0) {
      return fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user/common",
                         titleHi, titleLo);
   } else {
      return fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user/{:08X}",
                         titleHi, titleLo, persistentId);
   }
}

vfs::Path
getTitleSavePath(uint64_t title,
                 nn_act::SlotNo slot,
                 std::string_view path)
{
   return getTitleSaveDirectory(title, slot) / path;
}

} // namespace internal

void
Library::registerPathSymbols()
{
   RegisterFunctionExport(SAVEInit);
   RegisterFunctionExport(SAVEShutdown);
   RegisterFunctionExport(SAVEInitSaveDir);
   RegisterFunctionExport(SAVEGetSharedDataTitlePath);
   RegisterFunctionExport(SAVEGetSharedSaveDataPath);

   RegisterDataInternal(sSaveData);
}

} // namespace cafe::nn_save
