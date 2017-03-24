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

using FSAFileHandle = uint32_t;

class FSADevice : public IOSDevice
{
   using FSARequest = coreinit::FSARequest;
   using FSAResponse = coreinit::FSAResponse;

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
   fs::Path
   translatePath(const char *path) const;

   fs::File::OpenMode
   translateMode(const char *mode) const;

   FSAFileHandle
   mapFileHandle(fs::FileHandle handle);

   fs::FileHandle
   mapFileHandle(FSAFileHandle handle);

   bool
   removeFileHandle(FSAFileHandle handle);

private:
   IOSError
   changeDir(FSARequest *request,
             FSAResponse *response);

   IOSError
   openFile(FSARequest *request,
            FSAResponse *response);

   IOSError
   closeFile(FSARequest *request,
             FSAResponse *response);

   IOSError
   readFile(size_t vecIn,
            size_t vecOut,
            IOSVec *vec);

   IOSError
   writeFile(size_t vecIn,
             size_t vecOut,
             IOSVec *vec);

private:
   fs::FileSystem *mFS;
   fs::Path mWorkingPath;
   std::vector<fs::FileHandle> mOpenFiles;
};

/** @} */

} // namespace kernel
