#include "ios_acp.h"
#include "ios_acp_client_save.h"
#include "ios_acp_log.h"

#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/ios_stackobject.h"

#include <fmt/format.h>

namespace ios::acp::client::save
{

using namespace ios::fs;

static FSAStatus
checkExistenceUsingOpenDir(std::string_view path)
{
   // TODO: FSAOpenDir, FSACloseDir
   return FSAStatus::Cancelled;
}

static bool
checkExistence(std::string_view path)
{
   StackObject<FSAStat> stat;
   auto error = FSAGetInfoByQuery(internal::getFsaHandle(),
                                  path, FSAQueryInfoType::Stat, stat);
   if (error == FSAStatus::PermissionError) {
      error = checkExistenceUsingOpenDir(path);
   }

   if (error == FSAStatus::OK) {
      return true;
   }

   if (error == FSAStatus::NotFound) {
      return false;
   }

   // Unexpected error!
   return false;
}

static FSAStatus
createDir(std::string_view path, uint32_t mode)
{
   return FSAMakeDir(internal::getFsaHandle(), path, mode);
}

static FSAStatus
makeQuota(std::string_view path, uint32_t mode, uint64_t size)
{
   return FSAMakeQuota(internal::getFsaHandle(), path, mode, size);
}

static Error
createSystemSaveDir(std::string_view devicePath)
{
   auto path = fmt::format("{}{}", devicePath, "usr/save/system/");
   if (!checkExistence(path)) {
      createDir(path, 0x600);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/acp/");
   if (!checkExistence(path)) {
      makeQuota(path, 0x600, 0x800000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/act/");
   if (!checkExistence(path)) {
      makeQuota(path, 0x660, 0x800000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/fpd/");
   if (!checkExistence(path)) {
      makeQuota(path, 0x660, 0x200000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/boss/");
   if (!checkExistence(path)) {
      makeQuota(path, 0x660, 0x2000000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/nim/");
   if (!checkExistence(path)) {
      makeQuota(path, 0x660, 0x1A00000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/pdm");
   if (!checkExistence(path)) {
      makeQuota(path, 0x660, 0x1400000u);
   }

   path = fmt::format("{}{}", devicePath, "usr/save/system/no_delete");
   if (!checkExistence(path)) {
      makeQuota(path, 0x600, 0x800000u);
   }

   // TODO: FlushQuota devicePath/usr/save
   return Error::OK;
}

static Error
createNoDeleteDirs(std::string_view devicePath)
{
   auto path = fmt::format("{}{}{:08X}/",
                           devicePath,
                           "usr/save/system/no_delete/",
                           0x00050000);
   if (!checkExistence(path)) {
      createDir(path, 0x600);
   }

   path = fmt::format("{}{}{:08X}/{:08X}/",
                      devicePath,
                      "usr/save/system/no_delete/",
                      0x00050000, 0x10100D00);
   if (!checkExistence(path)) {
      createDir(path, 0x600);
   }

   // TODO: ChangeOwner for path
   return Error::OK;
}

static Error
createAppBoxCache()
{
   auto path = "/vol/storage_mlc01/usr/save/system/acp/appBoxCache/";
   if (!checkExistence(path)) {
      createDir(path, 0x600);
   }

   return Error::OK;
}

Error
start()
{
   auto error = createSystemSaveDir("/vol/storage_mlc01/");
   if (error) {
      internal::acpLog->error(
         "client::save::start: createSystemSaveDir failed with error = {}", error);
      return error;
   }

   error = createNoDeleteDirs("/vol/storage_mlc01/");
   if (error) {
      internal::acpLog->error(
         "client::save::start: createNoDeleteDirs failed with error = {}", error);
      return error;
   }

   error = createAppBoxCache();
   if (error) {
      internal::acpLog->error(
         "client::save::start: createAppBoxCache failed with error = {}", error);
      return error;
   }

   // TODO: StartTimeStampThread
   return Error::OK;
}

} // namespace ios::acp::client::save
