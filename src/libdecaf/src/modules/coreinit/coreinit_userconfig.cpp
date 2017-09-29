#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mutex.h"
#include "coreinit_userconfig.h"
#include "ppcutils/wfunc_call.h"

#include <fmt/format.h>

namespace coreinit
{

using ios::auxil::UCReadSysConfigRequest;
using ios::auxil::UCWriteSysConfigRequest;

struct UserConfigData
{
   OSMutex lock;

   static constexpr uint32_t SmallMessageCount = 0x100;
   static constexpr uint32_t SmallMessageSize = 0x80;

   static constexpr uint32_t LargeMessageCount = 0x40;
   static constexpr uint32_t LargeMessageSize = 0x1000;

   bool initialised;

   IPCBufPool *smallMessagePool;
   IPCBufPool *largeMessagePool;

   std::array<uint8_t, SmallMessageCount * SmallMessageSize> smallMessageBuffer;
   std::array<uint8_t, LargeMessageCount * LargeMessageSize> largeMessageBuffer;

   be_val<uint32_t> smallMessageCount;
   be_val<uint32_t> largeMessageCount;
};

static UserConfigData *
sUserConfigData;

static IOSAsyncCallbackFn
sIosAsyncCallbackFn = nullptr;

namespace internal
{

static void *
ucAllocateMessage(uint32_t size)
{
   void *message = nullptr;

   if (size == 0) {
      return nullptr;
   } else if (size <= UserConfigData::SmallMessageSize) {
      message = IPCBufPoolAllocate(sUserConfigData->smallMessagePool, size);
   } else {
      message = IPCBufPoolAllocate(sUserConfigData->largeMessagePool, size);
   }

   std::memset(message, 0, size);
   return message;
}


static void
ucFreeMessage(void *message)
{
   IPCBufPoolFree(sUserConfigData->smallMessagePool, message);
   IPCBufPoolFree(sUserConfigData->largeMessagePool, message);
}


static UCError
ucSetupAsyncParams(UCCommand command,
                   uint32_t unk_r4,
                   uint32_t count,
                   UCSysConfig *settings,
                   IOSVec *vecs,
                   UCAsyncParams *asyncParams)
{
   if (!settings || !vecs || !asyncParams) {
      return UCError::InvalidParam;
   }

   asyncParams->command = command;
   asyncParams->unk0x0C = unk_r4;
   asyncParams->count = count;
   asyncParams->settings = settings;
   asyncParams->vecs = vecs;
   return UCError::OK;
}


static UCError
ucHandleIosResult(UCError result,
                  UCCommand command,
                  uint32_t unk_r5,
                  uint32_t count,
                  UCSysConfig *settings,
                  IOSVec *vecs,
                  UCAsyncParams *asyncParams,
                  UCAsyncCallback callback,
                  void *callbackContext)
{
   if (!settings && !vecs) {
      if (result == UCError::OK) {
         return UCError::InvalidParam;
      } else {
         return static_cast<UCError>(result);
      }
   }

   if (result != UCError::NoIPCBuffers) {
      if (settings && vecs) {
         if (result == UCError::OK && asyncParams) {
            // Return as we have a pending async result...!
            return UCError::OK;
         }

         if (command == UCCommand::ReadSysConfig) {
            auto request = virt_ptr<UCReadSysConfigRequest> { vecs[0].vaddr };

            for (auto i = 0u; i < count; ++i) {
               settings[i].error = request->settings[i].error;

               if (settings[i].error) {
                  result = settings[i].error;
               }

               if (!settings[i].data) {
                  result = UCError::InvalidParam;
               }

               if (settings[i].error == UCError::OK) {
                  auto src = virt_ptr<void> { vecs[i + 1].vaddr };

                  switch (settings[i].dataSize) {
                  case 0:
                     continue;
                  case 1:
                     *virt_cast<uint8_t>(settings[i].data) = *virt_cast<uint8_t>(src);
                     break;
                  case 2:
                     *virt_cast<uint16_t>(settings[i].data) = *virt_cast<uint16_t>(src);
                     break;
                  case 4:
                     *virt_cast<uint32_t>(settings[i].data) = *virt_cast<uint32_t>(src);
                     break;
                  default:
                     std::memset(settings[i].data.getRawPointer(), 0, 4); // why???
                     std::memcpy(settings[i].data.getRawPointer(), src.getRawPointer(), settings[i].dataSize);
                  }
               }
            }
         } else if (command == UCCommand::WriteSysConfig) {
            auto request = virt_ptr<UCWriteSysConfigRequest> { vecs[0].vaddr };

            for (auto i = 0u; i < count; ++i) {
               settings[i].error = request->settings[i].error;

               if (settings[i].error) {
                  result = settings[i].error;
               }
            }
         } else {
            decaf_abort(fmt::format("Unimplemented result handler for UCCommand {}", command));
         }
      }

      if (callback) {
         callback(result, command, count, settings, callbackContext);
      }
   }

   if (vecs) {
      for (auto i = 0u; i < count + 1; ++i) {
         internal::ucFreeMessage(virt_ptr<void> { vecs[i].vaddr }.getRawPointer());
      }

      internal::ucFreeMessage(vecs);
   }

   return static_cast<UCError>(result);
}


static void
ucIosAsyncCallback(IOSError status, void *context)
{
   auto asyncParams = reinterpret_cast<UCAsyncParams *>(context);
   ucHandleIosResult(UCError::OK,
                     asyncParams->command,
                     asyncParams->unk0x0C,
                     asyncParams->count,
                     asyncParams->settings,
                     asyncParams->vecs,
                     nullptr,
                     asyncParams->callback,
                     asyncParams->context);
}

} // namespace internal


UCError
UCOpen()
{
   OSLockMutex(&sUserConfigData->lock);

   if (!sUserConfigData->initialised) {
      if (!sUserConfigData->smallMessagePool) {
         sUserConfigData->smallMessagePool = IPCBufPoolCreate(sUserConfigData->smallMessageBuffer.data(),
                                                              static_cast<uint32_t>(sUserConfigData->smallMessageBuffer.size()),
                                                              UserConfigData::SmallMessageSize,
                                                              &sUserConfigData->smallMessageCount,
                                                              1);
      }

      if (!sUserConfigData->largeMessagePool) {
         sUserConfigData->largeMessagePool = IPCBufPoolCreate(sUserConfigData->largeMessageBuffer.data(),
                                                              static_cast<uint32_t>(sUserConfigData->largeMessageBuffer.size()),
                                                              UserConfigData::LargeMessageSize,
                                                              &sUserConfigData->largeMessageCount,
                                                              1);
      }

      if (sUserConfigData->smallMessagePool && sUserConfigData->largeMessagePool) {
         sUserConfigData->initialised = true;
      }
   }

   OSUnlockMutex(&sUserConfigData->lock);

   if (!sUserConfigData->initialised) {
      return UCError::Error;
   }

   return static_cast<UCError>(IOS_Open("/dev/usr_cfg", IOSOpenMode::None));
}


UCError
UCClose(IOSHandle handle)
{
   return static_cast<UCError>(IOS_Close(handle));
}


UCError
UCDeleteSysConfig(IOSHandle handle,
                  uint32_t count,
                  UCSysConfig *settings)
{
   return UCError::OK;
}


UCError
UCReadSysConfig(IOSHandle handle,
                uint32_t count,
                UCSysConfig *settings)
{
   return UCReadSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCReadSysConfigAsync(IOSHandle handle,
                     uint32_t count,
                     UCSysConfig *settings,
                     UCAsyncParams *asyncParams)
{
   auto result = UCError::OK;
   uint32_t msgBufSize = 0, vecBufSize = 0;
   void *msgBuf = nullptr, *vecBuf = nullptr;
   UCReadSysConfigRequest *request = nullptr;
   IOSVec *vecs = nullptr;

   if (!settings) {
      result = UCError::InvalidParam;
      goto fail;
   }

   msgBufSize = static_cast<uint32_t>(count * sizeof(UCSysConfig) + sizeof(UCReadSysConfigRequest));
   msgBuf = internal::ucAllocateMessage(msgBufSize);
   if (!msgBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   request = reinterpret_cast<UCReadSysConfigRequest *>(msgBuf);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings, settings, sizeof(UCSysConfig) * count);

   vecBufSize = static_cast<uint32_t>((count + 1) * sizeof(IOSVec));
   vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   vecs = reinterpret_cast<IOSVec *>(vecBuf);
   vecs[0].vaddr = cpu::translate(msgBuf);
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].vaddr = cpu::translate(internal::ucAllocateMessage(size));
         if (!vecs[1 + i].vaddr) {
            result = UCError::NoIPCBuffers;
            goto fail;
         }
      } else {
         vecs[1 + i].vaddr = 0u;
      }
   }

   if (!asyncParams) {
      result = static_cast<UCError>(IOS_Ioctlv(handle,
                                               UCCommand::ReadSysConfig,
                                               0,
                                               count + 1,
                                               vecs));
   } else {
      internal::ucSetupAsyncParams(UCCommand::ReadSysConfig,
                                   0,
                                   count,
                                   settings,
                                   vecs,
                                   asyncParams);

      result = static_cast<UCError>(IOS_IoctlvAsync(handle,
                                                    UCCommand::ReadSysConfig,
                                                    0,
                                                    count + 1,
                                                    vecs,
                                                    sIosAsyncCallbackFn,
                                                    asyncParams));
   }

   goto out;

fail:
   if (msgBuf) {
      internal::ucFreeMessage(msgBuf);
      msgBuf = nullptr;
   }

   if (vecBuf) {
      for (auto i = 0u; i < count; ++i) {
         if (vecs[1 + i].vaddr) {
            internal::ucFreeMessage(virt_ptr<void> { vecs[1 + i].vaddr }.getRawPointer());
         }
      }

      internal::ucFreeMessage(vecBuf);
      vecBuf = nullptr;
      vecs = nullptr;
   }

out:
   return internal::ucHandleIosResult(result,
                                      UCCommand::ReadSysConfig,
                                      0,
                                      count,
                                      settings,
                                      vecs,
                                      asyncParams,
                                      nullptr,
                                      nullptr);
}


UCError
UCWriteSysConfig(IOSHandle handle,
                 uint32_t count,
                 UCSysConfig *settings)
{
   return UCWriteSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCWriteSysConfigAsync(IOSHandle handle,
                      uint32_t count,
                      UCSysConfig *settings,
                      UCAsyncParams *asyncParams)
{
   auto result = UCError::OK;
   uint32_t msgBufSize = 0, vecBufSize = 0;
   void *vecBuf = nullptr, *msgBuf = nullptr;
   UCWriteSysConfigRequest *request = nullptr;
   IOSVec *vecs = nullptr;

   if (!settings) {
      result = UCError::InvalidParam;
      goto fail;
   }

   msgBufSize = static_cast<uint32_t>(count * sizeof(UCSysConfig) + sizeof(UCWriteSysConfigRequest));
   msgBuf = internal::ucAllocateMessage(msgBufSize);
   if (!msgBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   request = reinterpret_cast<UCWriteSysConfigRequest *>(msgBuf);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings, settings, count * sizeof(UCSysConfig));

   vecBufSize = static_cast<uint32_t>((count + 1) * sizeof(IOSVec));
   vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   vecs = reinterpret_cast<IOSVec *>(vecBuf);
   vecs[0].vaddr = cpu::translate(msgBuf);
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].vaddr = cpu::translate(internal::ucAllocateMessage(size));
         if (!vecs[1 + i].vaddr) {
            result = UCError::NoIPCBuffers;
            goto fail;
         }
      } else {
         vecs[1 + i].vaddr = 0u;
      }
   }

   if (!asyncParams) {
      result = static_cast<UCError>(IOS_Ioctlv(handle,
                                               UCCommand::WriteSysConfig,
                                               0,
                                               count + 1,
                                               vecs));
   } else {
      internal::ucSetupAsyncParams(UCCommand::WriteSysConfig,
                                   0,
                                   count,
                                   settings,
                                   vecs,
                                   asyncParams);

      result = static_cast<UCError>(IOS_IoctlvAsync(handle,
                                                    UCCommand::WriteSysConfig,
                                                    0,
                                                    count + 1,
                                                    vecs,
                                                    sIosAsyncCallbackFn,
                                                    asyncParams));
   }

   goto out;

fail:
   if (msgBuf) {
      internal::ucFreeMessage(msgBuf);
      msgBuf = nullptr;
   }

   if (vecBuf) {
      for (auto i = 0u; i < count; ++i) {
         if (vecs[1 + i].vaddr) {
            internal::ucFreeMessage(virt_ptr<void> { vecs[1 + i].vaddr }.getRawPointer());
         }
      }

      internal::ucFreeMessage(vecBuf);
      vecBuf = nullptr;
      vecs = nullptr;
   }

out:
   return internal::ucHandleIosResult(result,
                                      UCCommand::WriteSysConfig,
                                      0,
                                      count,
                                      settings,
                                      vecs,
                                      asyncParams,
                                      nullptr,
                                      nullptr);
}


void
Module::initialiseUserConfig()
{
   OSInitMutex(&sUserConfigData->lock);
}

void
Module::registerUserConfigFunctions()
{
   RegisterKernelFunction(UCOpen);
   RegisterKernelFunction(UCClose);
   RegisterKernelFunction(UCReadSysConfig);
   RegisterKernelFunction(UCReadSysConfigAsync);
   RegisterKernelFunction(UCWriteSysConfig);
   RegisterKernelFunction(UCWriteSysConfigAsync);

   RegisterInternalData(sUserConfigData);
   RegisterInternalFunction(internal::ucIosAsyncCallback, sIosAsyncCallbackFn);
}

} // namespace coreinit
