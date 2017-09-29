#include "ios_auxil_usr_cfg_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_ipc.h"

namespace ios::auxil
{

using namespace kernel;

using UCHandle = kernel::ResourceHandleId;

static UCError
allocIpcData(uint32_t size,
             phys_ptr<void> *outData)
{
   auto buffer = IOS_HeapAlloc(CrossProcessHeapId,
                               size);

   if (!buffer) {
      return UCError::Alloc;
   }

   std::memset(buffer.getRawPointer(), 0, size);
   *outData = buffer;
   return UCError::OK;
}

static void
freeIpcData(phys_ptr<void> ipcData)
{
   IOS_HeapFree(CrossProcessHeapId, ipcData);
}

Error
UCOpen()
{
   return IOS_Open("/dev/usr_cfg", OpenMode::None);
}

Error
UCClose(UCHandle handle)
{
   return IOS_Close(handle);
}

UCError
UCReadSysConfig(UCHandle handle,
                uint32_t count,
                phys_ptr<UCSysConfig> settings)
{
   phys_ptr<void> vecBuffer = nullptr;
   phys_ptr<void> reqBuffer = nullptr;
   phys_ptr<UCReadSysConfigRequest> request = nullptr;
   phys_ptr<IoctlVec> vecs = nullptr;
   auto vecBufSize = static_cast<uint32_t>((1 + count) * sizeof(IoctlVec));
   auto reqBufSize = static_cast<uint32_t>(count * sizeof(UCSysConfig) + sizeof(UCReadSysConfigRequest));

   auto error = allocIpcData(vecBufSize, &vecBuffer);
   if (error < UCError::OK) {
      goto out;
   }

   error = allocIpcData(reqBufSize, &reqBuffer);
   if (error < UCError::OK) {
      goto out;
   }

   request = phys_cast<UCReadSysConfigRequest>(reqBuffer);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings, settings.getRawPointer(), sizeof(UCSysConfig) * count);

   vecs = phys_cast<IoctlVec>(vecBuffer);
   vecs[0].len = reqBufSize;
   vecs[0].paddr = reqBuffer;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size) {
         phys_ptr<void> dataBuffer;
         error = allocIpcData(vecBufSize, &dataBuffer);
         if (error < UCError::OK) {
            goto out;
         }

         vecs[1 + i].paddr = dataBuffer;
      } else {
         vecs[1 + i].paddr = phys_addr { 0u };
      }
   }

   error = static_cast<UCError>(IOS_Ioctlv(handle,
                                           UCCommand::ReadSysConfig,
                                           0,
                                           1 + count,
                                           vecs));

   for (auto i = 0u; i < count; ++i) {
      settings[i].error = request->settings[i].error;

      if (auto len = vecs[i + 1].len) {
         auto dst = phys_ptr<void> { phys_addr { virt_addr { settings[i].data }.getAddress() } };
         auto src = phys_ptr<const void> { vecs[i + 1].paddr };
         std::memcpy(dst.getRawPointer(), src.getRawPointer(), len);
      }
   }

out:
   for (auto i = 0u; i < count; ++i) {
      if (vecs[i + 1].paddr) {
         freeIpcData(vecs[i + 1].paddr);
      }
   }

   if (vecBuffer) {
      freeIpcData(vecBuffer);
   }

   if (reqBuffer) {
      freeIpcData(reqBuffer);
   }

   return error;
}

UCError
UCWriteSysConfig(UCHandle handle,
                 uint32_t count,
                 phys_ptr<UCSysConfig> settings)
{
   phys_ptr<void> vecBuffer = nullptr;
   phys_ptr<void> reqBuffer = nullptr;
   phys_ptr<UCWriteSysConfigRequest> request = nullptr;
   phys_ptr<IoctlVec> vecs = nullptr;
   auto vecBufSize = static_cast<uint32_t>((1 + count) * sizeof(IoctlVec));
   auto reqBufSize = static_cast<uint32_t>(count * sizeof(UCSysConfig) + sizeof(UCReadSysConfigRequest));

   auto error = allocIpcData(vecBufSize, &vecBuffer);
   if (error < UCError::OK) {
      goto out;
   }

   error = allocIpcData(reqBufSize, &reqBuffer);
   if (error < UCError::OK) {
      goto out;
   }

   request = phys_cast<UCWriteSysConfigRequest>(reqBuffer);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings, settings.getRawPointer(), sizeof(UCSysConfig) * count);

   vecs = phys_cast<IoctlVec>(vecBuffer);
   vecs[0].len = reqBufSize;
   vecs[0].paddr = reqBuffer;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size) {
         phys_ptr<void> dataBuffer;
         error = allocIpcData(vecBufSize, &dataBuffer);
         if (error < UCError::OK) {
            goto out;
         }

         vecs[1 + i].paddr = dataBuffer;

         auto src = phys_ptr<const void> { phys_addr { virt_addr { settings[i].data }.getAddress() } };
         auto dst = phys_ptr<void> { vecs[i + 1].paddr };
         std::memcpy(dst.getRawPointer(), src.getRawPointer(), size);
      } else {
         vecs[1 + i].paddr = phys_addr { 0u };
      }
   }

   error = static_cast<UCError>(IOS_Ioctlv(handle,
                                           UCCommand::WriteSysConfig,
                                           0,
                                           1 + count,
                                           vecs));

   for (auto i = 0u; i < count; ++i) {
      settings[i].error = request->settings[i].error;
   }

out:
   for (auto i = 0u; i < count; ++i) {
      if (vecs[i + 1].paddr) {
         freeIpcData(vecs[i + 1].paddr);
      }
   }

   if (vecBuffer) {
      freeIpcData(vecBuffer);
   }

   if (reqBuffer) {
      freeIpcData(reqBuffer);
   }

   return error;
}

} // namespace ios::auxil
