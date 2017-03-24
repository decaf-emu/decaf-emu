#include "kernel_ios_fsadevice.h"
#include "kernel_filesystem.h"
#include "modules/coreinit/coreinit_fs.h"
#include "modules/coreinit/coreinit_fsa_request.h"
#include "modules/coreinit/coreinit_fsa_response.h"

#include <cstring>

namespace kernel
{

using FSACommand = coreinit::FSACommand;
using FSARequest = coreinit::FSARequest;
using FSAResponse = coreinit::FSAResponse;
using FSAStatus = coreinit::FSAStatus;
using FSReadFlag = coreinit::FSReadFlag;
using FSWriteFlag = coreinit::FSWriteFlag;

IOSError
FSADevice::open(IOSOpenMode mode)
{
   mFS = getFileSystem();
   return IOSError::OK;
}

IOSError
FSADevice::close()
{
   return IOSError::OK;
}

IOSError
FSADevice::read(void *buffer,
                size_t length)
{
   return static_cast<IOSError>(FSAStatus::UnsupportedCmd);
}

IOSError
FSADevice::write(void *buffer,
                 size_t length)
{
   return static_cast<IOSError>(FSAStatus::UnsupportedCmd);
}

IOSError
FSADevice::ioctl(uint32_t cmd,
                 void *inBuf,
                 size_t inLen,
                 void *outBuf,
                 size_t outLen)
{
   auto request = reinterpret_cast<FSARequest *>(inBuf);
   auto response = reinterpret_cast<FSAResponse *>(outBuf);
   decaf_check(inLen == sizeof(FSARequest));
   decaf_check(outLen == sizeof(FSAResponse));

   if (request->emulatedError < 0) {
      return static_cast<IOSError>(request->emulatedError.value());
   }

   switch (static_cast<FSACommand>(cmd)) {
   case FSACommand::ChangeDir:
      return changeDir(request, response);
   case FSACommand::OpenFile:
      return openFile(request, response);
   case FSACommand::CloseFile:
      return closeFile(request, response);
   default:
      return static_cast<IOSError>(FSAStatus::UnsupportedCmd);
   }
}

IOSError
FSADevice::ioctlv(uint32_t cmd,
                  size_t vecIn,
                  size_t vecOut,
                  IOSVec *vec)
{
   auto request = be_ptr<FSARequest> { vec[0].vaddr };
   decaf_check(vec[0].len == sizeof(FSARequest));

   if (request->emulatedError < 0) {
      return static_cast<IOSError>(request->emulatedError.value());
   }

   switch (static_cast<FSACommand>(cmd)) {
   case FSACommand::ReadFile:
      return readFile(vecIn, vecOut, vec);
   case FSACommand::WriteFile:
      return writeFile(vecIn, vecOut, vec);
   default:
      return static_cast<IOSError>(FSAStatus::UnsupportedCmd);
   }
}

fs::Path
FSADevice::translatePath(const char *path) const
{
   if (path[0] == '/') {
      return path;
   } else {
      return mWorkingPath.join(path);
   }
}

fs::File::OpenMode
FSADevice::translateMode(const char *mode) const
{
   auto result = 0u;

   if (std::strchr(mode, 'r')) {
      result |= fs::File::Read;
   }

   if (std::strchr(mode, 'w')) {
      result |= fs::File::Write;
   }

   if (std::strchr(mode, 'a')) {
      result |= fs::File::Append;
   }

   if (std::strchr(mode, '+')) {
      result |= fs::File::Update;
   }

   return static_cast<fs::File::OpenMode>(result);
}

FSAFileHandle
FSADevice::mapFileHandle(fs::FileHandle handle)
{
   auto index = 0u;

   for (index = 0u; index < mOpenFiles.size(); ++index) {
      if (!mOpenFiles[index]) {
         mOpenFiles[index] = handle;
         break;
      }
   }

   if (index == mOpenFiles.size()) {
      mOpenFiles.push_back(handle);
   }

   return static_cast<FSAFileHandle>(index + 1);
}

fs::FileHandle
FSADevice::mapFileHandle(FSAFileHandle handle)
{
   if (handle == 0 || handle > mOpenFiles.size()) {
      return nullptr;
   }

   return mOpenFiles[handle - 1];
}

bool
FSADevice::removeFileHandle(FSAFileHandle handle)
{
   if (handle == 0 || handle > mOpenFiles.size()) {
      return false;
   }

   mOpenFiles[handle - 1] = nullptr;
   return true;
}

IOSError
FSADevice::changeDir(FSARequest *request,
                     FSAResponse *response)
{
   mWorkingPath = translatePath(request->changeDir.path);
   return IOSError::OK;
}

IOSError
FSADevice::openFile(FSARequest *request,
                    FSAResponse *response)
{
   auto path = translatePath(request->openFile.path);
   auto mode = translateMode(request->openFile.mode);
   auto fsHandle = mFS->openFile(path, mode);

   if (!fsHandle) {
      return static_cast<IOSError>(FSAStatus::NotFound);
   }

   auto fsaHandle = mapFileHandle(fsHandle);
   response->openFile.handle = fsaHandle;
   return IOSError::OK;
}

IOSError
FSADevice::closeFile(FSARequest *request,
                     FSAResponse *response)
{
   auto fh = mapFileHandle(request->closeFile.handle);

   if (!fh) {
      return static_cast<IOSError>(FSAStatus::InvalidFileHandle);
   }

   removeFileHandle(request->closeFile.handle);
   fh->close();
   return IOSError::OK;
}

IOSError
FSADevice::readFile(size_t vecIn,
                    size_t vecOut,
                    IOSVec *vec)
{
   decaf_check(vecIn == 1);
   decaf_check(vecOut == 2);

   auto request = be_ptr<FSARequest> { vec[0].vaddr };
   auto response = be_ptr<FSAResponse> { vec[2].vaddr };
   auto buffer = be_ptr<uint8_t> { vec[1].vaddr };
   auto readRequest = &request->readFile;
   auto fh = mapFileHandle(readRequest->handle);

   if (!fh) {
      return static_cast<IOSError>(FSAStatus::InvalidFileHandle);
   }

   if (readRequest->readFlags & FSReadFlag::ReadWithPos) {
      fh->seek(readRequest->pos);
   }

   auto elemsRead = fh->read(buffer, readRequest->size, readRequest->count);
   auto bytesRead = elemsRead * readRequest->size;
   return static_cast<IOSError>(bytesRead);
}


IOSError
FSADevice::writeFile(size_t vecIn,
                     size_t vecOut,
                     IOSVec *vec)
{
   decaf_check(vecIn == 2);
   decaf_check(vecOut == 1);

   auto request = be_ptr<FSARequest> { vec[0].vaddr };
   auto response = be_ptr<FSAResponse> { vec[2].vaddr };
   auto buffer = be_ptr<uint8_t> { vec[1].vaddr };
   auto writeRequest = &request->writeFile;
   auto fh = mapFileHandle(writeRequest->handle);

   if (!fh) {
      return static_cast<IOSError>(FSAStatus::InvalidFileHandle);
   }

   if (writeRequest->writeFlags & FSWriteFlag::WriteWithPos) {
      fh->seek(writeRequest->pos);
   }

   auto elemsWritten = fh->write(buffer, writeRequest->size, writeRequest->count);
   auto bytesWritten = elemsWritten * writeRequest->size;
   return static_cast<IOSError>(bytesWritten);
}

} // namespace kernel
