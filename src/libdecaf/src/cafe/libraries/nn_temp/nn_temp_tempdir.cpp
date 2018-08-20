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

using namespace cafe::coreinit;

#define TEMPLOGINFO(f, l, m, ...)                     \
   coreinit::internal::COSInfo(                       \
      COSReportModule::Unknown1,                      \
      "TEMP: [INFO]:%s(%d):" m, __VA_ARGS__, f, l);

#define TEMPLOGWARN(f, l, m, ...)                     \
   coreinit::internal::COSWarn(                       \
      COSReportModule::Unknown1,                      \
      "TEMP: [WARN]:%s(%d):" m, __VA_ARGS__, f, l);

#define TEMPLOGERR(f, l, m, ...)                      \
   coreinit::internal::COSError(                      \
      COSReportModule::Unknown1,                      \
      "TEMP: [ERROR]:%s(%d):" m, __VA_ARGS__, f, l);

namespace cafe::nn::temp
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

   std::snprintf(virt_addrof(deviceInfo->targetPath).getRawPointer(),
                 deviceInfo->targetPath.size(),
                 "%s/usr/tmp/app",
                 devicePath.getRawPointer());
}

static TEMPStatus
updatePreferentialDeviceInfo(virt_ptr<TEMPDeviceInfo> deviceInfo,
                             uint32_t maxSize,
                             TEMPDevicePreference devicePreference)
{
   StackObject<uint64_t> freeMlcSize;
   StackObject<uint64_t> freeUsbSize;

   FSGetFreeSpaceSize(virt_addrof(sTempDirData->fsClient),
                      virt_addrof(sTempDirData->fsCmdBlock),
                      make_stack_string("/vol/storage_mlc01"),
                      freeMlcSize,
                      FSErrorFlag::None);
   TEMPLOGINFO("UpdatePreferentialDeviceInfo", 544,
               "MLC freeSpaceSize={}", freeMlcSize);

   // TODO: Use nn::spm to find device index for USB
   if (FSGetFreeSpaceSize(virt_addrof(sTempDirData->fsClient),
                          virt_addrof(sTempDirData->fsCmdBlock),
                          make_stack_string("/vol/storage_usb01"),
                          freeUsbSize,
                          FSErrorFlag::All) != FSStatus::OK) {
      *freeUsbSize = 0ull;
   }
   TEMPLOGINFO("UpdatePreferentialDeviceInfo", 562,
               "USB freeSpaceSize={}", freeUsbSize);

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

      TEMPLOGINFO("UpdatePreferentialDeviceInfo", 648,
                  "Cannot create temp dir: ({})",  -12);
      return static_cast<TEMPStatus>(FSStatus::StorageFull);
   }

   TEMPLOGINFO("UpdatePreferentialDeviceInfo", 644,
               "Preferential Device Path for temp dir: {}",
               deviceInfo->targetPath);

   return TEMPStatus::OK;
}

static TEMPStatus
createAndMountTempDir(virt_ptr<TEMPDeviceInfo> deviceInfo)
{
   TEMPLOGINFO("CreateAndMountTempDir", 297,
               "(ENTR): dirID={}, targetPath={}",
               deviceInfo->dirId, deviceInfo->targetPath);

   auto error = static_cast<TEMPStatus>(
      FSMakeDir(virt_addrof(sTempDirData->fsClient),
                virt_addrof(sTempDirData->fsCmdBlock),
                virt_addrof(deviceInfo->targetPath),
                FSErrorFlag::All));
   TEMPLOGINFO("CreateAndMountTempDir", 305,
               "Make dir done at {}, returned {}",
               deviceInfo->targetPath, error);
   if (error == FSStatus::StorageFull) {
      goto out;
   }

   if (error != FSStatus::Exists) {
      error = static_cast<TEMPStatus>(
         FSChangeMode(virt_addrof(sTempDirData->fsClient),
                      virt_addrof(sTempDirData->fsCmdBlock),
                      virt_addrof(deviceInfo->targetPath),
                      0x666,
                      0x666,
                      FSErrorFlag::None));
      TEMPLOGINFO("CreateAndMountTempDir", 316,
                  "Change mode done at {}, returned {}",
                  deviceInfo->targetPath, error);
   }

   if (error = TEMPGetDirGlobalPath(deviceInfo->dirId,
                                    virt_addrof(sTempDirData->globalDirPath),
                                    GlobalPathMaxLength)) {
      goto out;
   }
   TEMPLOGINFO("CreateAndMountTempDir", 325,
               "Global Path={}", virt_addrof(sTempDirData->globalDirPath));

   error = static_cast<TEMPStatus>(
      FSMakeDir(virt_addrof(sTempDirData->fsClient),
                virt_addrof(sTempDirData->fsCmdBlock),
                virt_addrof(sTempDirData->globalDirPath),
                FSErrorFlag::All));
   TEMPLOGINFO("CreateAndMountTempDir", 333,
               "Make dir done at {}, returned {}",
               virt_addrof(sTempDirData->globalDirPath), error);
   if (error == FSStatus::StorageFull) {
      goto out;
   }

   if (error = TEMPGetDirPath(deviceInfo->dirId,
                              virt_addrof(sTempDirData->dirPath),
                              sTempDirData->dirPath.size())) {
      goto out;
   }
   TEMPLOGINFO("CreateAndMountTempDir", 346,
               "Dir Path={}", virt_addrof(sTempDirData->dirPath));

   error = static_cast<TEMPStatus>(
      FSBindMount(virt_addrof(sTempDirData->fsClient),
                  virt_addrof(sTempDirData->fsCmdBlock),
                  virt_addrof(sTempDirData->globalDirPath),
                  virt_addrof(sTempDirData->dirPath),
                  FSErrorFlag::None));
   TEMPLOGINFO("CreateAndMountTempDir", 353,
               "Bind mount done to {}, returned {}",
               virt_addrof(sTempDirData->dirPath), error);

out:
   TEMPLOGINFO("CreateAndMountTempDir", 356,
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
      std::snprintf(virt_addrof(deviceInfo->targetPath).getRawPointer(),
                    deviceInfo->targetPath.size(),
                    "/vol/storage_mlc%02d/usr/tmp/app",
                    deviceIndex);
   } else if (deviceType == TEMPDeviceType::USB) {
      std::snprintf(virt_addrof(deviceInfo->targetPath).getRawPointer(),
                    deviceInfo->targetPath.size(),
                    "/vol/storage_usb%02d/usr/tmp/app",
                    deviceIndex);
   } else {
      return static_cast<TEMPStatus>(FSStatus::NotFound);
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
   StackObject<FSDirHandle> handle;
   StackObject<FSDirEntry> entry;
   auto error = static_cast<TEMPStatus>(
      FSOpenDir(client, block, path, handle, FSErrorFlag::All));
   if (error) {
      TEMPLOGERR("RemoveDirectoryEntry", 264,
                 "FSOpenDir failed with {}", error);
      return error;
   }

   while (FSReadDir(client, block, *handle, entry, FSErrorFlag::All) == 0) {
      // Append directory entry name to path
      auto pathLen = strnlen(path.getRawPointer(), pathBufferSize);
      std::strncat(path.getRawPointer(),
                   "/",
                   pathBufferSize - pathLen - 1);

      std::strncat(path.getRawPointer(),
                   virt_addrof(entry->name).getRawPointer(),
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
      TEMPLOGWARN("RemoveRecursiveBody", 219,
                  "Quota is found inside temp directory!");
      // TODO: FSRemoveQuota
   } else if (statFlags & FSStatFlags::Directory) {
      error = removeDirectoryEntry(client, block, path, pathBufferSize);

      if (error == TEMPStatus::OK) {
         TEMPLOGINFO("RemoveRecursiveBody", 242,
                     "Removing {}", path);

         error = static_cast<TEMPStatus>(
            FSRemove(client, block, path, FSErrorFlag::All));
         if (error) {
            TEMPLOGERR("RemoveRecursiveBody", 236,
                       "FSRemove failed with {}", error);
         }
      }
   } else {
      TEMPLOGINFO("RemoveRecursiveBody", 242,
                  "Removing {}", path);

      error = static_cast<TEMPStatus>(
         FSRemove(client, block, path, FSErrorFlag::All));
      if (error) {
         TEMPLOGERR("RemoveRecursiveBody", 236,
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
   StackObject<FSStat> stat;
   auto error = static_cast<TEMPStatus>(
      FSGetStat(client, block, path, stat, FSErrorFlag::All));
   if (error) {
      TEMPLOGERR("RemoveRecursive", 202,
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
   TEMPLOGINFO("TeardownTempDir", 365,
               "(ENTR): path={}", virt_addrof(sTempDirData->globalDirPath));

   auto error =
      internal::removeRecursive(
         virt_addrof(sTempDirData->fsClient),
         virt_addrof(sTempDirData->fsCmdBlock),
         virt_addrof(sTempDirData->globalDirPath),
         sTempDirData->globalDirPath.size() - 1);
   if (error) {
      TEMPLOGERR("TeardownTempDir", 373,
                 "Failed to clean up temp dir, status={}", error);
   } else {
      FSBindUnmount(virt_addrof(sTempDirData->fsClient),
                    virt_addrof(sTempDirData->fsCmdBlock),
                    virt_addrof(sTempDirData->globalDirPath),
                    FSErrorFlag::None);
      TEMPLOGINFO("TeardownTempDir", 379,
                  "TEMP: [INFO]:%s(%d):Bind unmount done at {}",
                  virt_addrof(sTempDirData->globalDirPath));
   }

   TEMPLOGINFO("TeardownTempDir", 382, "(EXIT): return {}", error);
   return error;
}

static TEMPStatus
parseDirId(virt_ptr<const char> path,
           TEMPDirId *outDirId)
{
   if (!strstr(path.getRawPointer(), "/vol/storage_")) {
      return TEMPStatus::InvalidParam;
   }

   if (strstr(path.getRawPointer() + 13, "mlc")) {
      auto deviceIndex = strtoul(path.getRawPointer() + 16, nullptr, 10);
      auto deviceType = TEMPDeviceType::MLC;
      auto upid = 0u;

      if (auto pos = strstr(path.getRawPointer() + 16, "/usr/tmp/app")) {
         upid = strtoul(pos + 14, nullptr, 16);
      }

      *outDirId = makeDirId(deviceIndex, deviceType, upid);
   } else if (strstr(path.getRawPointer() + 13, "usb")) {
      auto deviceIndex = strtoul(path.getRawPointer() + 16, nullptr, 10);
      auto deviceType = TEMPDeviceType::USB;
      auto upid = 0u;

      if (auto pos = strstr(path.getRawPointer() + 16, "/usr/tmp/app")) {
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
   if (std::snprintf(virt_addrof(sTempDirData->globalDirPath).getRawPointer(),
                     GlobalPathMaxLength,
                     "%s/%08x",
                     rootPath.getRawPointer(),
                     OSGetUPID()) >= GlobalPathMaxLength) {
      coreinit::internal::OSPanic(
         "temp.cpp", 442,
         fmt::format("The specified path is too long: {}",
                     virt_addrof(sTempDirData->globalDirPath).getRawPointer()));
      return TEMPStatus::FatalError;
   }

   StackObject<FSDirHandle> handle;
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
      TEMPLOGERR("ForceRemoveTempDir", 478,
                 "Failed to clean up temp dir, status={}", error);
   }

   return error;
}

static void
tempShutdownBody(bool isDriverDone)
{
   TEMPLOGINFO("_TEMPShutdownBody", 695, "(ENTRY)")
   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (sTempDirData->initCount <= 0) {
      if (!isDriverDone) {
         TEMPLOGWARN("_TEMPShutdownBody", 749, "Library is not initialized.");
      }
   } else if (sTempDirData->initCount == 1) {
      auto error =
         forceRemoveTempDir(make_stack_string("/vol/storage_mlc01/usr/tmp/app"));
      if (error && error != FSStatus::NotFound) {
         TEMPLOGERR("_TEMPShutdownBody", 708, "Failed to delete temp dir in MLC.");
      }

      // TODO: Use nn::spm to find device index for USB
      error =
         forceRemoveTempDir(make_stack_string("/vol/storage_usb01/usr/tmp/app"));
      if (error && error != FSStatus::NotFound) {
         TEMPLOGERR("_TEMPShutdownBody", 729, "Failed to delete temp dir in USB.");
      }

      FSDelClient(virt_addrof(sTempDirData->fsClient), FSErrorFlag::None);
      sTempDirData->initCount = 0;
      TEMPLOGINFO("_TEMPShutdownBody", 737, "Deleted client");
   } else {
      sTempDirData->initCount--;
   }

   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   TEMPLOGINFO("_TEMPShutdownBody", 754, "(EXIT)")
}

} // namespace internal

TEMPStatus
TEMPInit()
{
   TEMPLOGINFO("TEMPInit", 668, "(ENTR)");

   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (!sTempDirData->initCount) {
      FSInit();
      FSAddClient(virt_addrof(sTempDirData->fsClient), FSErrorFlag::None);
      FSInitCmdBlock(virt_addrof(sTempDirData->fsCmdBlock));
   } else {
      TEMPLOGINFO("TEMPInit", 661, "Library is already initialized.");
   }

   sTempDirData->initCount++;
   OSUnlockMutex(virt_addrof(sTempDirData->mutex));

   TEMPLOGINFO("TEMPInit", 668, "(EXIT): return {}", 0);
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
   StackObject<TEMPDeviceInfo> deviceInfo;
   auto error = TEMPStatus::OK;

   TEMPLOGINFO("TEMPCreateAndInitTempDir", 779,
               "(ENTR): maxSize={}, pref={}", maxSize, devicePreference);
   OSLockMutex(virt_addrof(sTempDirData->mutex));

   if (!internal::checkIsInitialised()) {
      error = TEMPStatus::FatalError;
      goto out;
   }

   if (!outDirId) {
      TEMPLOGERR("TEMPCreateAndInitTempDir", 793,
                 "pDirID was NULL.");
      error = TEMPStatus::InvalidParam;
      goto out;
   }

   if (devicePreference != TEMPDevicePreference::LargestFreeSpace &&
       devicePreference != TEMPDevicePreference::USB) {
      TEMPLOGERR("TEMPCreateAndInitTempDir", 800,
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
   TEMPLOGINFO("TEMPCreateAndInitTempDir", 826,
               "(EXIT): return {}", error);
   return error;
}

TEMPStatus
TEMPGetDirPath(TEMPDirId dirId,
               virt_ptr<char> pathBuffer,
               uint32_t pathBufferSize)
{
   StackObject<TEMPDeviceInfo> deviceInfo;

   if (!internal::checkIsInitialised()) {
      return TEMPStatus::FatalError;
   }

   if (!pathBuffer) {
      TEMPLOGERR("TEMPGetDirPath", 865,
                 "path was NULL");
      return TEMPStatus::InvalidParam;
   }

   if (pathBufferSize < DirPathMaxLength) {
      TEMPLOGERR("TEMPGetDirPath", 870,
                 "pathLen(={}) was too short. Must be equal or bigger than TEMP_DIR_PATH_LENGTH_MAX(={})",
                 pathBufferSize, DirPathMaxLength);
      return TEMPStatus::InvalidParam;
   }

   if (std::snprintf(pathBuffer.getRawPointer(),
                     pathBufferSize,
                     "/vol/temp/%016llx",
                     dirId) >= DirPathMaxLength) {
      TEMPLOGERR("TEMPGetDirPath", 881,
                 "Failed to generate path");
      return TEMPStatus::FatalError;
   }

   return TEMPStatus::OK;
}

TEMPStatus
TEMPGetDirGlobalPath(TEMPDirId dirId,
                     virt_ptr<char> pathBuffer,
                     uint32_t pathBufferSize)
{
   StackObject<TEMPDeviceInfo> deviceInfo;

   if (!internal::checkIsInitialised()) {
      return static_cast<TEMPStatus>(FSStatus::FatalError);
   }

   if (!pathBuffer) {
      TEMPLOGERR("TEMPGetDirGlobalPath", 898,
                 "path was NULL");
      return TEMPStatus::InvalidParam;
   }

   if (pathBufferSize < GlobalPathMaxLength) {
      TEMPLOGERR("TEMPGetDirGlobalPath", 903,
                 "pathLen(={}) was too short. Must be equal or bigger than TEMP_DIR_PATH_LENGTH_MAX(={})",
                 pathBufferSize, DirPathMaxLength);
      return TEMPStatus::InvalidParam;
   }

   if (auto error = internal::getDeviceInfo(deviceInfo, dirId)) {
      return error;
   }

   if (std::snprintf(pathBuffer.getRawPointer(),
                     pathBufferSize,
                     "%s/%08x",
                     virt_addrof(deviceInfo->targetPath).getRawPointer(),
                     static_cast<uint32_t>(deviceInfo->dirId & 0xFFFFFFFF)) >= GlobalPathMaxLength) {
      TEMPLOGERR("TEMPGetDirGlobalPath", 922,
                 "Failed to generate path");
      return TEMPStatus::FatalError;
   }

   return TEMPStatus::OK;
}

TEMPStatus
TEMPShutdownTempDir(TEMPDirId id)
{
   TEMPLOGINFO("TEMPShutdownTempDir", 834,
               "(ENTR): dirID={}", id);

   OSLockMutex(virt_addrof(sTempDirData->mutex));
   if (!internal::checkIsInitialised()) {
      OSUnlockMutex(virt_addrof(sTempDirData->mutex));
      return TEMPStatus::FatalError;
   }

   auto error = internal::teardownTempDir(id);
   if (error && error != FSStatus::NotFound) {
      TEMPLOGERR("TEMPShutdownTempDir", 848,
                 "Failed to delete temp dir ({}).", id);
   }

   OSUnlockMutex(virt_addrof(sTempDirData->mutex));
   TEMPLOGINFO("TEMPShutdownTempDir", 853,
               "(EXIT): return {}", error);
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
}

} // namespace cafe::nn::temp
