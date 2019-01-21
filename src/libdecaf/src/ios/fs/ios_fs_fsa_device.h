#pragma once
#include "ios_fs_enum.h"
#include "ios_fs_fsa_request.h"
#include "ios_fs_fsa_response.h"
#include "ios_fs_fsa_types.h"

#include "ios/ios_device.h"
#include "vfs/vfs_device.h"
#include "vfs/vfs_permissions.h"

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
   struct Handle
   {
      enum Type
      {
         Unused,
         File,
         Directory
      };

      Handle() :
         type(Unused)
      {
      }

      Handle(std::unique_ptr<vfs::FileHandle> file) :
         type(File),
         file(std::move(file))
      {
      }

      Handle(vfs::DirectoryIterator itr) :
         type(Directory),
         directory(std::move(itr))
      {
      }

      Type type;
      std::unique_ptr<vfs::FileHandle> file;
      vfs::DirectoryIterator directory;
   };

public:
   FSADevice();

   FSAStatus changeDir(vfs::User user, phys_ptr<FSARequestChangeDir> request);
   FSAStatus closeDir(vfs::User user, phys_ptr<FSARequestCloseDir> request);
   FSAStatus closeFile(vfs::User user, phys_ptr<FSARequestCloseFile> request);
   FSAStatus flushFile(vfs::User user, phys_ptr<FSARequestFlushFile> request);
   FSAStatus flushQuota(vfs::User user, phys_ptr<FSARequestFlushQuota> request);
   FSAStatus getCwd(vfs::User user, phys_ptr<FSAResponseGetCwd> response);
   FSAStatus getInfoByQuery(vfs::User user, phys_ptr<FSARequestGetInfoByQuery> request, phys_ptr<FSAResponseGetInfoByQuery> response);
   FSAStatus getPosFile(vfs::User user, phys_ptr<FSARequestGetPosFile> request, phys_ptr<FSAResponseGetPosFile> response);
   FSAStatus isEof(vfs::User user, phys_ptr<FSARequestIsEof> request);
   FSAStatus makeDir(vfs::User user, phys_ptr<FSARequestMakeDir> request);
   FSAStatus makeQuota(vfs::User user, phys_ptr<FSARequestMakeQuota> request);
   FSAStatus mount(vfs::User user, phys_ptr<FSARequestMount> request);
   FSAStatus mountWithProcess(vfs::User user, phys_ptr<FSARequestMountWithProcess> request);
   FSAStatus openDir(vfs::User user, phys_ptr<FSARequestOpenDir> request, phys_ptr<FSAResponseOpenDir> response);
   FSAStatus openFile(vfs::User user, phys_ptr<FSARequestOpenFile> request, phys_ptr<FSAResponseOpenFile> response);
   FSAStatus readDir(vfs::User user, phys_ptr<FSARequestReadDir> request, phys_ptr<FSAResponseReadDir> response);
   FSAStatus readFile(vfs::User user, phys_ptr<FSARequestReadFile> request, phys_ptr<uint8_t> buffer, uint32_t bufferLen);
   FSAStatus remove(vfs::User user, phys_ptr<FSARequestRemove> request);
   FSAStatus rename(vfs::User user, phys_ptr<FSARequestRename> request);
   FSAStatus rewindDir(vfs::User user, phys_ptr<FSARequestRewindDir> request);
   FSAStatus setPosFile(vfs::User user, phys_ptr<FSARequestSetPosFile> request);
   FSAStatus statFile(vfs::User user, phys_ptr<FSARequestStatFile> request, phys_ptr<FSAResponseStatFile> response);
   FSAStatus truncateFile(vfs::User user, phys_ptr<FSARequestTruncateFile> request);
   FSAStatus unmount(vfs::User user, phys_ptr<FSARequestUnmount> request);
   FSAStatus unmountWithProcess(vfs::User user, phys_ptr<FSARequestUnmountWithProcess> request);
   FSAStatus writeFile(vfs::User user, phys_ptr<FSARequestWriteFile> request, phys_ptr<const uint8_t> buffer, uint32_t bufferLen);

private:
   FSAStatus
   translateError(vfs::Error error) const;

   vfs::Path
   translatePath(phys_ptr<const char> path) const;

   vfs::FileHandle::Mode
   translateMode(phys_ptr<const char> mode) const;

   void
   translateStat(const vfs::Status &entry,
                 phys_ptr<FSAStat> stat) const;

   int32_t
   mapHandle(std::unique_ptr<vfs::FileHandle> file);

   int32_t
   mapHandle(vfs::DirectoryIterator folder);

   FSAStatus
   mapFileHandle(int32_t handle,
                 Handle *&file);

   FSAStatus
   mapFolderHandle(int32_t handle,
                   Handle *&folder);

   FSAStatus
   removeHandle(int32_t index,
                Handle::Type type);

private:
   std::shared_ptr<vfs::Device> mFS;
   vfs::Path mWorkingPath = "/";
   std::vector<Handle> mHandles;
};

/** @} */

} // namespace ios::fs::internal
