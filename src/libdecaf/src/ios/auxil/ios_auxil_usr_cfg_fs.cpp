#include "ios_auxil_usr_cfg_fs.h"

#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/fs/ios_fs_fsa_ipc.h"

using namespace ios::fs;
using namespace ios::kernel;

namespace ios::auxil::internal
{

static ResourceHandleId
sFsaHandle;

Error
UCInitFSA()
{
   auto error = IOS_Open("/dev/fsa", OpenMode::None);
   if (error < Error::OK) {
      return Error::NoResource;
   }

   sFsaHandle = static_cast<ResourceHandleId>(error);
   return Error::OK;
}


phys_ptr<uint8_t>
UCAllocFileData(uint32_t size)
{
   auto ptr = IOS_HeapAllocAligned(CrossProcessHeapId, size, 0x40u);
   if (ptr) {
      std::memset(ptr.getRawPointer(), 0, size);
   }

   return phys_cast<uint8_t>(ptr);
}


void
UCFreeFileData(phys_ptr<uint8_t> fileData,
               uint32_t size)
{
   IOS_HeapFree(CrossProcessHeapId, fileData);
}


UCError
UCReadConfigFile(std::string_view filename,
                 uint32_t *outSize,
                 phys_ptr<uint8_t> *outData)
{
   StackObject<FSAStat> stat;
   FSAFileHandle fileHandle;

   auto status = FSAOpenFile(sFsaHandle, filename, "r", &fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileOpen;
   }

   status = FSAStatFile(sFsaHandle, fileHandle, stat);
   if (status < FSAStatus::OK) {
      FSACloseFile(sFsaHandle, fileHandle);
      return UCError::FileStat;
   }

   auto fileData = UCAllocFileData(stat->size);
   if (!fileData) {
      FSACloseFile(sFsaHandle, fileHandle);
      return UCError::Alloc;
   }

   status = FSAReadFile(sFsaHandle,
                        fileData,
                        stat->size,
                        1,
                        fileHandle,
                        FSAReadFlag::None);
   if (status < FSAStatus::OK) {
      UCFreeFileData(fileData, stat->size);
      FSACloseFile(sFsaHandle, fileHandle);
      return UCError::FileRead;
   }

   status = FSACloseFile(sFsaHandle, fileHandle);
   if (status < FSAStatus::OK) {
      UCFreeFileData(fileData, stat->size);
      return UCError::FileClose;
   }

   *outSize = stat->size;
   *outData = fileData;
   return UCError::OK;
}


UCError
UCWriteConfigFile(std::string_view filename,
                  phys_ptr<uint8_t> buffer,
                  uint32_t size)
{
   StackObject<FSAStat> stat;
   FSAFileHandle fileHandle;

   if (!buffer) {
      // REMOVE
   }

   auto status = FSAOpenFile(sFsaHandle, filename, "w", &fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileOpen;
   }


   status = FSAWriteFile(sFsaHandle,
                         buffer,
                         size,
                         1,
                         fileHandle,
                         FSAWriteFlag::None);
   if (status < FSAStatus::OK) {
      FSACloseFile(sFsaHandle, fileHandle);
      return UCError::FileWrite;
   }

   status = FSACloseFile(sFsaHandle, fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileClose;
   }

   return UCError::OK;
}

} // namespace ios::auxil::internal
