#pragma once
#include "kernel_ios_device.h"
#include "kernel_filesystem.h"
#include "modules/coreinit/coreinit_fs.h"
#include "modules/coreinit/coreinit_fsa_request.h"
#include "modules/coreinit/coreinit_fsa_response.h"

#include <cstdint>

namespace kernel
{

/**
 * \ingroup kernel_ios
 * @{
 */

struct FSAHandle
{
   enum Type
   {
      Unused,
      File,
      Folder
   };

   Type type;
   fs::FileHandle file;
   fs::FolderHandle folder;
};

using coreinit::FSAStatus;
using coreinit::FSStat;

using coreinit::FSARequest;
using coreinit::FSARequestChangeDir;
using coreinit::FSARequestCloseDir;
using coreinit::FSARequestCloseFile;
using coreinit::FSARequestFlushFile;
using coreinit::FSARequestFlushQuota;
using coreinit::FSARequestGetInfoByQuery;
using coreinit::FSARequestGetPosFile;
using coreinit::FSARequestIsEof;
using coreinit::FSARequestMakeDir;
using coreinit::FSARequestMount;
using coreinit::FSARequestOpenDir;
using coreinit::FSARequestOpenFile;
using coreinit::FSARequestReadDir;
using coreinit::FSARequestReadFile;
using coreinit::FSARequestRemove;
using coreinit::FSARequestRename;
using coreinit::FSARequestRewindDir;
using coreinit::FSARequestSetPosFile;
using coreinit::FSARequestStatFile;
using coreinit::FSARequestTruncateFile;
using coreinit::FSARequestUnmount;
using coreinit::FSARequestWriteFile;

using coreinit::FSAResponse;
using coreinit::FSAResponseGetCwd;
using coreinit::FSAResponseGetFileBlockAddress;
using coreinit::FSAResponseGetPosFile;
using coreinit::FSAResponseGetVolumeInfo;
using coreinit::FSAResponseGetInfoByQuery;
using coreinit::FSAResponseOpenDir;
using coreinit::FSAResponseOpenFile;
using coreinit::FSAResponseReadDir;
using coreinit::FSAResponseStatFile;

class FSADevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/fsa";

public:
   virtual IOSError
   open(IOSOpenMode mode) override;

   virtual IOSError
   close() override;

   virtual IOSError
   read(void *buffer,
        size_t length) override;

   virtual IOSError
   write(void *buffer,
         size_t length) override;

   virtual IOSError
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual IOSError
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IOSVec *vec) override;

private:
   FSAStatus
   translateError(fs::Error error) const;

   fs::Path
   translatePath(const char *path) const;

   fs::File::OpenMode
   translateMode(const char *mode) const;

   void
   translateStat(const fs::FolderEntry &entry,
                 FSStat *stat) const;

   uint32_t
   mapHandle(fs::FileHandle file);

   uint32_t
   mapHandle(fs::FolderHandle folder);

   FSAStatus
   mapHandle(uint32_t handle,
             fs::FileHandle &file);

   FSAStatus
   mapHandle(uint32_t handle,
             fs::FolderHandle &folder);

   FSAStatus
   removeHandle(uint32_t index,
                FSAHandle::Type type);

private:
   FSAStatus changeDir(FSARequestChangeDir *request);
   FSAStatus closeDir(FSARequestCloseDir *request);
   FSAStatus closeFile(FSARequestCloseFile *request);
   FSAStatus flushFile(FSARequestFlushFile *request);
   FSAStatus flushQuota(FSARequestFlushQuota *request);
   FSAStatus getCwd(FSAResponseGetCwd *response);
   FSAStatus getInfoByQuery(FSARequestGetInfoByQuery *request, FSAResponseGetInfoByQuery *response);
   FSAStatus getPosFile(FSARequestGetPosFile *request, FSAResponseGetPosFile *response);
   FSAStatus isEof(FSARequestIsEof *request);
   FSAStatus makeDir(FSARequestMakeDir *request);
   FSAStatus mount(FSARequestMount *request);
   FSAStatus openDir(FSARequestOpenDir *request, FSAResponseOpenDir *response);
   FSAStatus openFile(FSARequestOpenFile *request, FSAResponseOpenFile *response);
   FSAStatus readDir(FSARequestReadDir *request, FSAResponseReadDir *response);
   FSAStatus readFile(FSARequestReadFile *request, uint8_t *buffer, uint32_t bufferLen);
   FSAStatus remove(FSARequestRemove *request);
   FSAStatus rename(FSARequestRename *request);
   FSAStatus rewindDir(FSARequestRewindDir *request);
   FSAStatus setPosFile(FSARequestSetPosFile *request);
   FSAStatus statFile(FSARequestStatFile *request, FSAResponseStatFile *response);
   FSAStatus truncateFile(FSARequestTruncateFile *request);
   FSAStatus unmount(FSARequestUnmount *request);
   FSAStatus writeFile(FSARequestWriteFile *request, const uint8_t *buffer, uint32_t bufferLen);

private:
   fs::FileSystem *mFS;
   fs::Path mWorkingPath;
   std::vector<FSAHandle> mHandles;
};

/** @} */

} // namespace kernel
