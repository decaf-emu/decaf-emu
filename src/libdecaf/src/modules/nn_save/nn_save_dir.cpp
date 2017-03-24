#include "filesystem/filesystem.h"
#include "kernel/kernel_filesystem.h"
#include "nn_save.h"
#include "nn_save_dir.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/nn_act/nn_act_core.h"

namespace nn
{

namespace save
{

SaveStatus
SAVEInitSaveDir(uint8_t userID)
{
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

   // Create current user save folder
   auto savePath = internal::getSaveDirectory(userID);

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


void
Module::registerDirFunctions()
{
   RegisterKernelFunction(SAVEInitSaveDir);
   RegisterKernelFunction(SAVEGetSharedDataTitlePath);
   RegisterKernelFunction(SAVEGetSharedSaveDataPath);
}

namespace internal
{

fs::Path
getSaveDirectory(uint32_t account)
{
   if (account == nn::act::CurrentUserSlot) {
      account = nn::act::GetSlotNo();
   }

   return fmt::format("/vol/save/{}", account);
}

fs::Path
getSavePath(uint32_t account,
            const char *path)
{
   if (account == nn::act::CurrentUserSlot) {
      account = nn::act::GetSlotNo();
   }

   return fmt::format("/vol/save/{}/{}", account, path);
}

} // namespace internal

} // namespace save

} // namespace nn
