#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/nn_act/nn_act_core.h"
#include "nn_save.h"
#include "nn_save_core.h"

namespace nn
{

namespace save
{

static bool
sSaveInitialised = false;

SaveStatus
SAVEInit()
{
   if (sSaveInitialised) {
      return SaveStatus::OK;
   }

   auto fs = kernel::getFileSystem();
   auto titleID = coreinit::OSGetTitleID();
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);

   // Create title save folder
   auto path = fmt::format("/vol/storage_mlc01/usr/save/{:08x}/{:08x}/user",
                           titleHi, titleLo);
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

   sSaveInitialised = true;
   return SaveStatus::OK;
}

void
SAVEShutdown()
{
   // TODO: Must we unmount /vol/save ?
   sSaveInitialised = false;
}

SaveStatus
SAVEInitSaveDir(uint8_t userID)
{
   decaf_check(sSaveInitialised);

   // Create user's save folder
   auto savePath = internal::getSaveDirectory(userID);
   auto fs = kernel::getFileSystem();

   if (!fs->makeFolder(savePath)) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


SaveStatus
SAVEGetSharedDataTitlePath(uint64_t titleID,
                           const char *dir,
                           char *buffer,
                           uint32_t bufferSize)
{
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = snprintf(buffer, bufferSize,
                          "/vol/storage_mlc01/sys/title/%08x/%08x/content/%s",
                          titleHi, titleLo, dir);

   if (result < 0 || static_cast<uint32_t>(result) >= bufferSize) {
      return SaveStatus::FatalError;
   }

   return SaveStatus::OK;
}


SaveStatus
SAVEGetSharedSaveDataPath(uint64_t titleID,
                          const char *dir,
                          char *buffer,
                          uint32_t bufferSize)
{
   auto titleLo = static_cast<uint32_t>(titleID & 0xffffffff);
   auto titleHi = static_cast<uint32_t>(titleID >> 32);
   auto result = snprintf(buffer, bufferSize,
                          "/vol/storage_mlc01/usr/save/%08x/%08x/user/common/%s",
                          titleHi, titleLo, dir);

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
getSavePath(uint32_t account,
            const char *path)
{
   return getSaveDirectory(account).join(path);
}

} // namespace internal

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(SAVEInit);
   RegisterKernelFunction(SAVEShutdown);
   RegisterKernelFunction(SAVEInitSaveDir);
   RegisterKernelFunction(SAVEGetSharedDataTitlePath);
   RegisterKernelFunction(SAVEGetSharedSaveDataPath);
}

} // namespace save

} // namespace nn
