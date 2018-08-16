#include "nn_save.h"
#include "nn_save_path.h"

#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/nn_act/nn_act_stubs.h"
#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"

#include <fmt/format.h>

namespace cafe::nn::save
{

struct StaticPathData
{
   be2_val<bool> initialised;
};

static virt_ptr<StaticPathData>
sPathData = nullptr;

SaveStatus
SAVEInit()
{
   if (sPathData->initialised) {
      return SaveStatus::OK;
   }

   // Create title save folder
   auto fs = ::kernel::getFileSystem();
   auto titleID = coreinit::OSGetTitleID();
   auto path = internal::getTitleSaveRoot(titleID);
   auto titleFolder = fs->makeFolder(path);

   if (!titleFolder) {
      return SaveStatus::FatalError;
   }

   // Mount title save folder to /vol/save
   if (!fs->makeLink("/vol/save", titleFolder)) {
      return SaveStatus::FatalError;
   }

   // Create /vol/save/common
   auto savePath = internal::getSaveDirectory(nn::act::SystemSlot);

   if (!fs->makeFolder(savePath)) {
      return SaveStatus::FatalError;
   }

   sPathData->initialised = true;
   return SaveStatus::OK;
}

void
SAVEShutdown()
{
   // TODO: Must we unmount /vol/save ?
   sPathData->initialised = false;
}

SaveStatus
SAVEInitSaveDir(uint8_t userID)
{
   decaf_check(sPathData->initialised);

   // Create user's save folder
   auto savePath = internal::getSaveDirectory(userID);
   auto fs = ::kernel::getFileSystem();

   if (!fs->makeFolder(savePath)) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID,
                           virt_ptr<const char> dir,
                           virt_ptr<char> buffer,
                           uint32_t bufferSize)
{
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = snprintf(buffer.getRawPointer(), bufferSize,
                          "/vol/storage_mlc01/sys/title/%08x/%08x/content/%s",
                          titleHi, titleLo, dir.getRawPointer());

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
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = snprintf(buffer.getRawPointer(), bufferSize,
                          "/vol/storage_mlc01/usr/save/%08x/%08x/user/common/%s",
                          titleHi, titleLo, dir.getRawPointer());

   if (result < 0 || static_cast<uint32_t>(result) >= bufferSize) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


namespace internal
{

fs::Path
getSaveDirectory(uint32_t slot)
{
   if (slot == nn::act::SystemSlot) {
      return fmt::format("/vol/save/common");
   } else if (slot == nn::act::CurrentUserSlot) {
      slot = nn::act::GetSlotNo();
   }

   return fmt::format("/vol/save/{:08X}", nn::act::GetPersistentIdEx(slot));
}

fs::Path
getSavePath(uint32_t slot,
            std::string_view path)
{
   return getSaveDirectory(slot).join(path);
}

fs::Path
getTitleSaveRoot(uint64_t title)
{
   auto titleLo = static_cast<uint32_t>(title & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(title >> 32);
   return fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user", titleHi, titleLo);
}

fs::Path
getTitleSaveDirectory(uint64_t title,
                      uint32_t slot)
{
   auto root = getTitleSaveRoot(title);

   if (slot == nn::act::SystemSlot) {
      return root.join("common");
   } else if (slot == nn::act::CurrentUserSlot) {
      slot = nn::act::GetSlotNo();
   }

   return root.join(fmt::format("{:08X}", nn::act::GetPersistentIdEx(slot)));
}

fs::Path
getTitleSavePath(uint64_t title,
                 uint32_t slot,
                 std::string_view path)
{
   return getTitleSaveDirectory(title, slot).join(path);
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

   RegisterDataInternal(sPathData);
}

} // namespace cafe::nn::save
