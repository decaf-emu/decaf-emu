#pragma once
#include "fsa_enum.h"
#include "fsa_request.h"
#include "fsa_response.h"
#include "fsa_types.h"
#include "ios/dev/ios_device.h"
#include "kernel/kernel_filesystem.h"

#include <cstdint>

namespace ios
{

namespace dev
{

namespace fsa
{

/**
 * \defgroup ios_dev_fsa /dev/fsa
 * \ingroup ios_dev
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

class FSADevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/fsa";

public:
   virtual Error
   open(OpenMode mode) override;

   virtual Error
   close() override;

   virtual Error
   read(void *buffer,
        size_t length) override;

   virtual Error
   write(void *buffer,
         size_t length) override;

   virtual Error
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual Error
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IoctlVec *vec) override;

private:
   FSAStatus
   translateError(fs::Error error) const;

   fs::Path
   translatePath(const char *path) const;

   fs::File::OpenMode
   translateMode(const char *mode) const;

   void
   translateStat(const fs::FolderEntry &entry,
                 FSAStat *stat) const;

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
   fs::FileSystem *mFS = nullptr;
   fs::Path mWorkingPath = "/";
   std::vector<FSAHandle> mHandles;
};

/** @} */

} // namespace fsa

} // namespace dev

} // namespace ios
