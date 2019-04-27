#include "nn_temp.h"
#include "nn_temp_tempdir.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_cosreport.h"
#include "cafe/libraries/coreinit/coreinit_fs_client.h"
#include "cafe/libraries/coreinit/coreinit_fs_cmdblock.h"
#include "cafe/libraries/coreinit/coreinit_fs_cmd.h"
#include "cafe/libraries/coreinit/coreinit_mutex.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_osreport.h"

#include <cinttypes>
#include <fmt/format.h>

using namespace cafe::coreinit;

template<typename... Args>
void tempLogInfo(const char *file, unsigned line, const char *msg,
                 const Args &... args)
{
   auto str = fmt::format(msg, args...);
   cafe::coreinit::internal::COSInfo(
      cafe::coreinit::COSReportModule::Unknown1,
      fmt::format("TEMP: [INFO]:{}({}):{}", file, line, str.c_str()));
}

template<typename... Args>
void tempLogWarn(const char *file, unsigned line, const char *msg,
                 const Args &... args)
{
   auto str = fmt::format(msg, args...);
   cafe::coreinit::internal::COSInfo(
      cafe::coreinit::COSReportModule::Unknown1,
      fmt::format("TEMP: [WARN]:{}({}):{}", file, line, str.c_str()));
}

template<typename... Args>
void tempLogError(const char *file, unsigned line, const char *msg,
                  const Args &... args)
{
   auto str = fmt::format(msg, args...);
   cafe::coreinit::internal::COSInfo(
      cafe::coreinit::COSReportModule::Unknown1,
      fmt::format("TEMP: [ERROR]:{}({}):{}", file, line, str.c_str()));
}

namespace cafe::nn_temp
{

struct TempDirData
{
   be2_val<int32_t> initCount;
   be2_struct<OSMutex> mutex;
   be2_struct<FSClient> fsClient;
   be2_struct<FSCmdBlock> fsCmdBlock;
   be2_array<char, FSMaxPathLength> globalDirPath;
   be2_array<char, DirPathMaxLength> dirPath;
};

static virt_ptr<TempDirData>
sTempDirData = nullptr;

namespace internal
{

bool
checkIsInitialised()
{
   if (sTempDirData->initCount > 0) {
      return true;
   }

   coreinit::internal::OSPanic(
      "temp.cpp", 181,
      "TEMP: [PANIC]:TEMP library is not initialized. Call TEMPInit() prior to this function.");
   return false;
}

static TEMPDirId
makeDirId(uint32_t deviceIndex,
          TEMPDeviceType deviceType,
          uint32_t upid)
{
   return deviceIndex | (deviceType << 8) |
      static_cast<uint64_t>(OSGetUPID()) << 32;
}

static void
setDeviceInfo(virt_ptr<TEMPDeviceInfo> deviceInfo,
              TEMPDeviceType deviceType,
              uint8_t deviceIndex,
              virt_ptr<char> devicePath)
{
   deviceInfo->dirId = makeDirId(deviceIndex, deviceType,
                                 static_cast<uint32_t>(OSGetUPID()));

   std::snprintf(virt_addrof(deviceInfo->targetPath).get(),
                 deviceInfo->targetPath.size(),
                 "%s/usr/tmp/app",
                 devicePath.get());
}

static TEMPStatus
updatePreferentialDeviceInfo(virt_ptr<TEMPDeviceInfo> deviceInfo,
                             uint32_t maxSize,
                             TEMPDevicePreference devicePreference)
{
   auto freeMlcSize = StackObject<uint64_t> { };
   auto freeUsbSize = StackObject<uint64_t> { };

   FSGetFreeSpaceSize(virt_addrof(sTempDirData->fsClient),
                      virt_addrof(sTempDirData->fsCmdBlock),
                      make_stack_string("/vol/storage_mlc01"),
                      freeMlcSize,
                      FSErrorFlag::None);
   tempLogInfo("UpdatePreferentialDeviceInfo", 544,
               "MLC freeSpaceSize={}", *freeMlcSize);

   // TODO: Use nn::spm to find device index for USB
   if (FSGetFreeSpaceSize(virt_addrof(sTempDirData->fsClient),
                          virt_addrof(sTempDirData->fsCmdBlock),
                          make_stack_string("/vol/storage_usb01"),
                          freeUsbSize,
                          FSErrorFlag::All) != FSStatus::OK) {
      *freeUsbSize = 0ull;
   }
   tempLogInfo("UpdatePreferentialDeviceInfo", 562,
               "USB freeSpaceSize={}", *freeUsbSize);

   if ((devicePreference == TEMPDevicePreference::USB
        || *freeUsbSize >= *freeMlcSize)
       && (*freeUsbSize >= maxSize)) {
      setDeviceInfo(deviceInfo, TEMPDeviceType::USB, 1,
                    make_stack_string("/vol/storage_usb01"));
   } else if (*freeMlcSize >= maxSize) {
      setDeviceInfo(deviceInfo, TEMPDeviceType::MLC, 1,
                    make_stack_string("/vol/storage_mlc01"));
   } else {
      setDeviceInfo(deviceInfo, TEMPDeviceType::Invalid, 0,
                    make_stack_string(""));

      tempLogInfo("UpdatePreferentialDeviceInfo", 648,
                  "Cannot create temp dir: ({})",  -12);
      return static_cast<TEMPStatus>(TEMPStatus::StorageFull);
   }

   tempLogInfo("UpdatePreferentialDeviceInfo", 644,
               "Preferential Device Path for temp dir: {}",
               virt_addrof(deviceInfo->targetPath));

   return TEMPStatus::OK;
}

static TEMPStatus
createAndMountTempDir(virt_ptr<TEMPDeviceInfo> deviceInfo)
{
   tempLogInfo("CreateAndMountTempDir", 297,
               "(ENTR): dirID={}, targetPath={}",
               deviceInfo->dirId, virt_addrof(deviceInfo->targetPath));

   auto error = static_cast<TEMPStatus>(
      FSMakeDir(virt_addrof(sTempDirData->fsClient),
                virt_addrof(sTempDirData->fsCmdBlock),
                virt_addrof(deviceInfo->targetPath),
                FSErrorFlag::All));
   tempLogInfo("CreateAndMountTempDir", 305,
               "Make dir done at {}, returned {}",
               virt_addrof(deviceInfo->targetPath), error);
   if (error == TEMPStatus::StorageFull) {
      goto out;
   }

   if (error != TEMPStatus::Exists) {
      error = static_cast<TEMPStatus>(
         FSChangeMode(virt_addrof(sTempDirData->fsClient),
                      virt_addrof(sTempDirData->fsCmdBlock),
                      virt_addrof(deviceInfo->targetPath),
                      0x666,
                      0x666,
                      FSErrorFlag::None));
      tempLogInfo("CreateAndMountTempDir", 316,
                  "Change mode done at {}, returned {}",
                  virt_addrof(deviceInfo->targetPath), error);
   }

   error = TEMPGetDirGlobalPath(deviceInfo->dirId,
                                virt_addrof(sTempDirData->globalDirPath),
                                GlobalPathMaxLength);
   if (error != TEMPStatus::OK) {
      goto out;
   }
   tempLogInfo("CreateAndMountTempDir", 325,
               "Global Path={}", virt_addrof(sTempDirData->globalDirPath));

   error = static_cast<TEMPStatus>(
      FSMakeDir(virt_addrof(sTempDirData->fsClient),
                virt_addrof(sTempDirData->fsCmdBlock),
                virt_addrof(sTempDirData->globalDirPath),
                FSErrorFlag::All));
   tempLogInfo("CreateAndMountTempDir", 333,
               "Make dir done at {}, returned {}",
               virt_addrof(sTempDirData->globalDirPath), error);
   if (error == TEMPStatus::StorageFull) {
      goto out;
   }

   error = TEMPGetDirPath(deviceInfo->dirId,
                          virt_addrof(sTempDirData->dirPath),
                          sTempDirData->dirPath.size());
   if (error != TEMPStatus::OK) {
      goto out;
   }
   tempLogInfo("CreateAndMountTempDir", 346,
               "Dir Path={}", virt_addrof(sTempDirData->dirPath));

   error = static_cast<TEMPStatus>(
      FSBindMount(virt_addrof(sTempDirData->fsClient),
                  virt_addrof(sTempDirData->fsCmdBlock),
                  virt_addrof(sTempDirData->globalDirPath),
                  virt_addrof(sTempDirData->dirPath),
                  FSErrorFlag::None));
   tempLogInfo("CreateAndMountTempDir", 353,
               "Bind mount done to {}, returned {}",
               virt_addrof(sTempDirData->dirPath), error);

out:
   tempLogInfo("CreateAndMountTempDir", 356,
               "(EXIT): return {}", error);
   return error;
}

static TEMPDeviceType
getDeviceType(TEMPDirId dirId)
{
   return static_cast<TEMPDeviceType>((dirId >> 8) & 0xFF);
}

static TEMPDeviceType
getDeviceIndex(TEMPDirId dirId)
{
   return static_cast<TEMPDeviceType>(dirId & 0xFF);
}

static TEMPStatus
getDeviceInfo(virt_ptr<TEMPDeviceInfo> deviceInfo,
              TEMPDirId dirId)
{
   auto deviceType = getDeviceType(dirId);
   auto deviceIndex = getDeviceIndex(dirId);
   deviceInfo->dirId = dirId;

   if (deviceType == TEMPDeviceType::MLC) {
      std::snprintf(virt_addrof(deviceInfo->targetPath).get(),
                    deviceInfo->targetPath.size(),
                    "/vol/storage_mlc%02d/usr/tmp/app",
                    deviceIndex);
   } else if (deviceType == TEMPDeviceType::USB) {
      std::snprintf(virt_addrof(deviceInfo->targetPath).get(),
                    deviceInfo->targetPath.size(),
                    "/vol/storage_usb%02d/usr/tmp/app",
                    deviceIndex);
   } else {
      return static_cast<TEMPStatus>(TEMPStatus::NotFound);
   }

   return TEMPStatus::OK;
}

static TEMPStatus
removeRecursiveBody(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    virt_ptr<char> path,
                    uint32_t pathBufferSize,
                    FSStatFlags statFlags);

static TEMPStatus
removeDirectoryEntry(virt_ptr<FSClient> client,
                     virt_ptr<FSCmdBlock> block,
                     virt_ptr<char> path,
                     uint32_t pathBufferSize)
{
   auto handle = StackObject<FSDirHandle> { };
   auto entry = StackObject<FSDirEntry> { };
   auto error = static_cast<TEMPStatus>(
      FSOpenDir(client, block, path, handle, FSErrorFlag::All));
   if (error) {
      tempLogError("RemoveDirectoryEntry", 264,
                 "FSOpenDir failed with {}", error);
      return error;
   }

   while (FSReadDir(client, block, *handle, entry, FSErrorFlag::All) == 0) {
      // Append directory entry name to path
      auto pathLen = strnlen(path.get(), pathBufferSize);
      std::strncat(path.get(),
                   "/",
                   pathBufferSize - pathLen - 1);

      std::strncat(path.get(),
                   virt_addrof(entry->name).get(),
                   pathBufferSize - pathLen - 2);

      error = removeRecursiveBody(client, block, path, pathBufferSize,
                                  entry->stat.flags);

      // Reset path
      path[pathLen] = char { 0 };

      if (error) {
         break;
      }
   }

   FSCloseDir(client, block, *handle, FSErrorFlag::None);
   return error;
}

static TEMPStatus
removeRecursiveBody(virt_ptr<FSClient> client,
                    virt_ptr<FSCmdBlock> block,
                    virt_ptr<char> path,
                    uint32_t pathBufferSize,
                    FSStatFlags statFlags)
{
   auto error = TEMPStatus::OK;

   if (statFlags & FSStatFlags::Quota) {
      tempLogWarn("RemoveRecursiveBody", 219,
                  "Quota is found inside temp directory!");
      // TODO: FSRemoveQuota
   } else if (statFlags & FSStatFlags::Directory) {
      error = removeDirectoryEntry(client, block, path, pathBufferSize);

      if (error == TEMPStatus::OK) {
         tempLogInfo("RemoveRecursiveBody", 242,
                     "Removing {}", path);

         error = static_cast<TEMPStatus>(
            FSRemove(client, block, path, FSErrorFlag::All));
         if (error) {
            tempLogError("RemoveRecursiveBody", 236,
                       "FSRemove failed with {}", error);
         }
      }
   } else {
      tempLogInfo("RemoveRecursiveBody", 242,
                  "Removing {}", path);

      error = static_cast<TEMPStatus>(
         FSRemove(client, block, path, FSErrorFlag::All));
      if (error) {
         tempLogError("RemoveRecursiveBody", 236,
                    "FSRemove failed with {}", error);
      }
   }

   return error;
}

static TEMPStatus
removeRecursive(virt_ptr<FSClient> client,
                virt_ptr<FSCmdBlock> block,
                virt_ptr<char> path,
                uint32_t pathBufferSize)
{
   auto stat = StackObject<FSStat> { };
   auto error = static_cast<TEMPStatus>(
      FSGetStat(client, block, path, stat, FSErrorFlag::All));
   if (error) {
      tempLogError("RemoveRecursive", 202,
                 "FSGetStat failed with {}", error);
      return error;
   }

   return removeRecursiveBody(client, block, path, pathBufferSize, stat->flags);
}

static TEMPStatus
teardownTempDir(TEMPDirId id)
{
   TEMPGetDirPath(id,
                  virt_addrof(sTempDirData->globalDirPath),
                  GlobalPathMaxLength);
   tempLogInfo("TeardownTempDir", 365,
               "(ENTR): path={}", virt_addrof(sTempDirData->globalDirPath));

   auto error =
      internal::removeRecursive(
         virt_addrof(sTempDirData->fsClient),
         virt_addrof(sTempDirData->fsCmdBlock),
         virt_addrof(sTempDirData->globalDirPath),
         sTempDirData->globalDirPath.size() - 1);
   if (error) {
      tempLogError("TeardownTempDir", 373,
                 "Failed to clean up temp dir, status={}", error);
   } else {
      FSBindUnmount(virt_addrof(sTempDirData->fsClient),
                    virt_addrof(sTempDirData->fsCmdBlock),
                    virt_addrof(sTempDirData->globalDirPath),
                    FSErrorFlag::None);
      tempLogInfo("TeardownTempDir", 379,
                  "TEMP: [INFO]:%s(%d):Bind unmount done at {}",
                  virt_addrof(sTempDirData->globalDirPath));
   }

   tempLogInfo("TeardownTempDir", 382, "(EXIT): return {}", error);
   return error;
}

static TEMPStatus
parseDirId(virt_ptr<const char> path,
           TEMPDirId *outDirId)
{
   if (!strstr(path.get(), "/vol/storage_")) {
      return TEMPStatus::InvalidParam;
   }

   if (strstr(path.get() + 13, "mlc")) {
      auto deviceIndex = strtoul(path.get() + 16, nullptr, 10);
      auto deviceType = TEMPDeviceType::MLC;
      auto upid = 0u;

      if (auto pos = strstr(path.get() + 16, "/usr/tmp/app")) {
         upid = strtoul(pos + 14, nullptr, 16);
      }

      *outDirId = makeDirId(deviceIndex, deviceType, upid);
   } else if (strstr(path.get() + 13, "usb")) {
      auto deviceIndex = strtoul(path.get() + 16, nullptr, 10);
      auto deviceType = TEMPDeviceType::USB;
      auto upid = 0u;

      if (auto pos = strstr(path.get() + 16, "/usr/tmp/app")) {
         upid = strtoul(pos + 14, nullptr, 16);
      }

      *outDirId = makeDirId(deviceIndex, deviceType, upid);
   } else {
      return TEMPStatus::InvalidParam;
   }

   return TEMPStatus::OK;
}

static TEMPStatus
forceRemoveTempDir(virt_ptr<const char> rootPath)
{
   if (std::snprintf(virt_addrof(sTempDirData->globalDirPath).get(),
                     GlobalPathMaxLength,
                     "%s/%08x",
                     rootPath.get(),
                     static_cast<uint32_t>(OSGetUPID())) >= GlobalPathMaxLength) {
      coreinit::internal::OSPanic(
         "temp.cpp", 442,
         fmt::format("The specified path is too long: {}",
                     virt_addrof(sTempDirData->globalDirPath).get()));
      return TEMPStatus::FatalError;
   }

   auto handle = StackObject<FSDirHandle> { };
   auto error = static_cast<TEMPStatus>(
      FSOpenDir(virt_addrof(sTempDirData->fsClient),
                virt_addrof(sTempDirData->fsCmdBlock),
                rootPath,
                handle,
                FSErrorFlag::NotFound));
   if (error) {
      return error;
   }

   FSCloseDir(virt_addrof(sTempDirData->fsClient),
              virt_addrof(sTempDirData->fsCmdBlock),
              *handle,
              FSErrorFlag::None);

   auto dirId = TEMPDirId { };
   error = parseDirId(virt_addrof(sTempDirData->globalDirPath), &dirId);
   if (error) {
      return error;
   }

   error = TEMPGetDirPath(dirId, virt_addrof(sTempDirData->dirPath),
                          DirPathMaxLength);
   if (error) {
      return error;
   }

   FSBindUnmount(virt_addrof(sTempDirData->fsClient),
                 virt_addrof(sTempDirData->fsCmdBlock),
                 virt_addrof(sTempDirData->dirPath),
                 FSErrorFlag::None);

   error = internal::removeRecursive(virt_addrof(sTempDirData->fsClient),
                                     virt_addrof(sTempDirData->fsCmdBlock),
                                     virt_addrof(sTempDirData->globalDirPath),
                                     sTempDirData->globalDirPath.size() - 1);
   if (error) {
      tempLogError("ForceRemoveTempDir", 478,
                 "Failed to clean up temp dir, status={}", error);
   }

   return error;
}

static void
tempShutdownBody(bool isDriverDone)
{
   tempLogInfo("_TEMPShutdownBody", 695, "(ENTRY)");
   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (sTempDirData->initCount <= 0) {
      if (!isDriverDone) {
         tempLogWarn("_TEMPShutdownBody", 749,
                     "Library is not initialized.");
      }
   } else if (sTempDirData->initCount == 1) {
      auto error =
         forceRemoveTempDir(make_stack_string("/vol/storage_mlc01/usr/tmp/app"));
      if (error && error != TEMPStatus::NotFound) {
         tempLogError("_TEMPShutdownBody", 708,
                    "Failed to delete temp dir in MLC.");
      }

      // TODO: Use nn::spm to find device index for USB
      error =
         forceRemoveTempDir(make_stack_string("/vol/storage_usb01/usr/tmp/app"));
      if (error && error != TEMPStatus::NotFound) {
         tempLogError("_TEMPShutdownBody", 729,
                    "Failed to delete temp dir in USB.");
      }

      FSDelClient(virt_addrof(sTempDirData->fsClient), FSErrorFlag::None);
      sTempDirData->initCount = 0;
      tempLogInfo("_TEMPShutdownBody", 737,
                  "Deleted client");
   } else {
      sTempDirData->initCount--;
   }

   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   tempLogInfo("_TEMPShutdownBody", 754, "(EXIT)");
}

} // namespace internal

TEMPStatus
TEMPInit()
{
   tempLogInfo("TEMPInit", 668, "(ENTR)");
   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (!sTempDirData->initCount) {
      FSInit();
      FSAddClient(virt_addrof(sTempDirData->fsClient), FSErrorFlag::None);
      FSInitCmdBlock(virt_addrof(sTempDirData->fsCmdBlock));
   } else {
      tempLogInfo("TEMPInit", 661, "Library is already initialized.");
   }

   sTempDirData->initCount++;
   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   tempLogInfo("TEMPInit", 668, "(EXIT): return {}", 0);
   return TEMPStatus::OK;
}

void
TEMPShutdown()
{
   internal::tempShutdownBody(false);
}

TEMPStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         TEMPDevicePreference devicePreference,
                         virt_ptr<TEMPDirId> outDirId)
{
   auto deviceInfo = StackObject<TEMPDeviceInfo> { };
   auto error = TEMPStatus::OK;

   tempLogInfo("TEMPCreateAndInitTempDir", 779,
               "(ENTR): maxSize={}, pref={}", maxSize, devicePreference);
   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (!internal::checkIsInitialised()) {
      error = TEMPStatus::FatalError;
      goto out;
   }

   if (!outDirId) {
      tempLogError("TEMPCreateAndInitTempDir", 793,
                 "pDirID was NULL.");
      error = TEMPStatus::InvalidParam;
      goto out;
   }

   if (devicePreference != TEMPDevicePreference::LargestFreeSpace &&
       devicePreference != TEMPDevicePreference::USB) {
      tempLogError("TEMPCreateAndInitTempDir", 800,
                 "Invalid value was specified to pref.");
      error = TEMPStatus::InvalidParam;
      goto out;
   }

   error = internal::updatePreferentialDeviceInfo(deviceInfo,
                                                  maxSize,
                                                  devicePreference);
   if (error < TEMPStatus::OK) {
      goto out;
   }

   error = internal::createAndMountTempDir(deviceInfo);
   if (error < TEMPStatus::OK) {
      goto out;
   }

   *outDirId = deviceInfo->dirId;

out:
   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   tempLogInfo("TEMPCreateAndInitTempDir", 826,
               "(EXIT): return {}", error);
   return error;
}

TEMPStatus
TEMPGetDirPath(TEMPDirId dirId,
               virt_ptr<char> pathBuffer,
               uint32_t pathBufferSize)
{
   auto deviceInfo = StackObject<TEMPDeviceInfo> { };

   if (!internal::checkIsInitialised()) {
      return TEMPStatus::FatalError;
   }

   if (!pathBuffer) {
      tempLogError("TEMPGetDirPath", 865,
                 "path was NULL");
      return TEMPStatus::InvalidParam;
   }

   if (pathBufferSize < DirPathMaxLength) {
      tempLogError("TEMPGetDirPath", 870,
                   "pathLen(={}) was too short. Must be equal or bigger than TEMP_DIR_PATH_LENGTH_MAX(={})",
                   pathBufferSize, DirPathMaxLength);
      return TEMPStatus::InvalidParam;
   }

   if (std::snprintf(pathBuffer.get(),
                     pathBufferSize,
                     "/vol/temp/%016" PRIx64,
                     dirId) >= DirPathMaxLength) {
      tempLogError("TEMPGetDirPath", 881, "Failed to generate path");
      return TEMPStatus::FatalError;
   }

   return TEMPStatus::OK;
}

TEMPStatus
TEMPGetDirGlobalPath(TEMPDirId dirId,
                     virt_ptr<char> pathBuffer,
                     uint32_t pathBufferSize)
{
   auto deviceInfo = StackObject<TEMPDeviceInfo> { };

   if (!internal::checkIsInitialised()) {
      return static_cast<TEMPStatus>(TEMPStatus::FatalError);
   }

   if (!pathBuffer) {
      tempLogError("TEMPGetDirGlobalPath", 898,
                 "path was NULL");
      return TEMPStatus::InvalidParam;
   }

   if (pathBufferSize < GlobalPathMaxLength) {
      tempLogError("TEMPGetDirGlobalPath", 903,
                   "pathLen(={}) was too short. Must be equal or bigger than TEMP_DIR_PATH_LENGTH_MAX(={})",
                   pathBufferSize, DirPathMaxLength);
      return TEMPStatus::InvalidParam;
   }

   if (auto error = internal::getDeviceInfo(deviceInfo, dirId)) {
      return error;
   }

   if (std::snprintf(pathBuffer.get(),
                     pathBufferSize,
                     "%s/%08x",
                     virt_addrof(deviceInfo->targetPath).get(),
                     static_cast<uint32_t>(deviceInfo->dirId & 0xFFFFFFFF)) >= GlobalPathMaxLength) {
      tempLogError("TEMPGetDirGlobalPath", 922, "Failed to generate path");
      return TEMPStatus::FatalError;
   }

   return TEMPStatus::OK;
}

TEMPStatus
TEMPShutdownTempDir(TEMPDirId id)
{
   tempLogInfo("TEMPShutdownTempDir", 834, "(ENTR): dirID={}", id);

   OSLockMutex(virt_addrof(sTempDirData->mutex));
   if (!internal::checkIsInitialised()) {
      OSUnlockMutex(virt_addrof(sTempDirData->mutex));
      return TEMPStatus::FatalError;
   }

   auto error = internal::teardownTempDir(id);
   if (error && error != TEMPStatus::NotFound) {
      tempLogError("TEMPShutdownTempDir", 848,
                   "Failed to delete temp dir ({}).", id);
   }

   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   tempLogInfo("TEMPShutdownTempDir", 853, "(EXIT): return {}", error);
   return error;
}

void
Library::registerTempDirSymbols()
{
   RegisterFunctionExport(TEMPInit);
   RegisterFunctionExport(TEMPShutdown);
   RegisterFunctionExport(TEMPCreateAndInitTempDir);
   RegisterFunctionExport(TEMPGetDirPath);
   RegisterFunctionExport(TEMPGetDirGlobalPath);
   RegisterFunctionExport(TEMPShutdownTempDir);

   RegisterDataInternal(sTempDirData);
}

} // namespace cafe::nn_temp
