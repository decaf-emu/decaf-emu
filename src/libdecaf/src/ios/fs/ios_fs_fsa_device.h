#pragma once
#include "ios_fs_enum.h"
#include "ios_fs_fsa_request.h"
#include "ios_fs_fsa_response.h"
#include "ios_fs_fsa_types.h"
#include "ios/ios_device.h"
#include "kernel/kernel_filesystem.h"

#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::fs::internal
{

/**
 * \ingroup ios_fs
 * @{
 */

class FSADevice
{
   using FSError = ::fs::Error;
   using Path = ::fs::Path;
   using FileSystem = ::fs::FileSystem;
   using FileHandle = ::fs::FileHandle;
   using FolderEntry = ::fs::FolderEntry;
   using FolderHandle = ::fs::FolderHandle;

   struct Handle
   {
      enum Type
      {
         Unused,
         File,
         Folder
      };

      Type type;
      FileHandle file;
      FolderHandle folder;
   };

public:
   FSAStatus changeDir(phys_ptr<FSARequestChangeDir> request);
   FSAStatus closeDir(phys_ptr<FSARequestCloseDir> request);
   FSAStatus closeFile(phys_ptr<FSARequestCloseFile> request);
   FSAStatus flushFile(phys_ptr<FSARequestFlushFile> request);
   FSAStatus flushQuota(phys_ptr<FSARequestFlushQuota> request);
   FSAStatus getCwd(phys_ptr<FSAResponseGetCwd> response);
   FSAStatus getInfoByQuery(phys_ptr<FSARequestGetInfoByQuery> request, phys_ptr<FSAResponseGetInfoByQuery> response);
   FSAStatus getPosFile(phys_ptr<FSARequestGetPosFile> request, phys_ptr<FSAResponseGetPosFile> response);
   FSAStatus isEof(phys_ptr<FSARequestIsEof> request);
   FSAStatus makeDir(phys_ptr<FSARequestMakeDir> request);
   FSAStatus mount(phys_ptr<FSARequestMount> request);
   FSAStatus openDir(phys_ptr<FSARequestOpenDir> request, phys_ptr<FSAResponseOpenDir> response);
   FSAStatus openFile(phys_ptr<FSARequestOpenFile> request, phys_ptr<FSAResponseOpenFile> response);
   FSAStatus readDir(phys_ptr<FSARequestReadDir> request, phys_ptr<FSAResponseReadDir> response);
   FSAStatus readFile(phys_ptr<FSARequestReadFile> request, phys_ptr<uint8_t> buffer, uint32_t bufferLen);
   FSAStatus remove(phys_ptr<FSARequestRemove> request);
   FSAStatus rename(phys_ptr<FSARequestRename> request);
   FSAStatus rewindDir(phys_ptr<FSARequestRewindDir> request);
   FSAStatus setPosFile(phys_ptr<FSARequestSetPosFile> request);
   FSAStatus statFile(phys_ptr<FSARequestStatFile> request, phys_ptr<FSAResponseStatFile> response);
   FSAStatus truncateFile(phys_ptr<FSARequestTruncateFile> request);
   FSAStatus unmount(phys_ptr<FSARequestUnmount> request);
   FSAStatus writeFile(phys_ptr<FSARequestWriteFile> request, phys_ptr<const uint8_t> buffer, uint32_t bufferLen);

private:
   FSAStatus
   translateError(FSError error) const;

   Path
   translatePath(phys_ptr<const char> path) const;

   ::fs::File::OpenMode
   translateMode(phys_ptr<const char> mode) const;

   void
   translateStat(const FolderEntry &entry,
                 phys_ptr<FSAStat> stat) const;

   int32_t
   mapHandle(FileHandle file);

   int32_t
   mapHandle(FolderHandle folder);

   FSAStatus
   mapHandle(int32_t handle,
             FileHandle &file);

   FSAStatus
   mapHandle(int32_t handle,
             FolderHandle &folder);

   FSAStatus
   removeHandle(int32_t index,
                Handle::Type type);

private:
   FileSystem *mFS = nullptr;
   Path mWorkingPath = "/";
   std::vector<Handle> mHandles;
};

/** @} */

} // namespace ios::fs::internal
