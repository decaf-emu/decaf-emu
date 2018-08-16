#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mutex.h"
#include "coreinit_userconfig.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"

#include <fmt/format.h>
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

using ios::auxil::UCReadSysConfigRequest;
using ios::auxil::UCWriteSysConfigRequest;

static constexpr uint32_t SmallMessageCount = 0x100;
static constexpr uint32_t SmallMessageSize = 0x80;

static constexpr uint32_t LargeMessageCount = 0x40;
static constexpr uint32_t LargeMessageSize = 0x1000;

struct StaticUserConfigData
{
   be2_struct<OSMutex> lock;

   be2_val<BOOL> initialised = FALSE;

   be2_virt_ptr<IPCBufPool> smallMessagePool = nullptr;
   be2_virt_ptr<IPCBufPool> largeMessagePool = nullptr;

   be2_array<uint8_t, SmallMessageCount * SmallMessageSize> smallMessageBuffer;
   be2_array<uint8_t, LargeMessageCount * LargeMessageSize> largeMessageBuffer;

   be2_val<uint32_t> smallMessageCount = 0;
   be2_val<uint32_t> largeMessageCount = 0;
};

static virt_ptr<StaticUserConfigData> sUserConfigData = nullptr;
static IOSAsyncCallbackFn sUcIosAsyncCallback = nullptr;

namespace internal
{

static virt_ptr<void>
ucAllocateMessage(uint32_t size)
{
   auto message = virt_ptr<void> { nullptr };

   if (size == 0) {
      return nullptr;
   } else if (size <= SmallMessageSize) {
      message = IPCBufPoolAllocate(sUserConfigData->smallMessagePool, size);
   } else {
      message = IPCBufPoolAllocate(sUserConfigData->largeMessagePool, size);
   }

   std::memset(message.getRawPointer(), 0, size);
   return message;
}


static void
ucFreeMessage(virt_ptr<void> message)
{
   IPCBufPoolFree(sUserConfigData->smallMessagePool, message);
   IPCBufPoolFree(sUserConfigData->largeMessagePool, message);
}


static UCError
ucSetupAsyncParams(UCCommand command,
                   uint32_t unk_r4,
                   uint32_t count,
                   virt_ptr<UCSysConfig> settings,
                   virt_ptr<IOSVec> vecs,
                   virt_ptr<UCAsyncParams> asyncParams)
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
                  virt_ptr<UCSysConfig> settings,
                  virt_ptr<IOSVec> vecs,
                  virt_ptr<UCAsyncParams> asyncParams,
                  UCAsyncCallbackFn callback,
                  virt_ptr<void> callbackContext)
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
            auto request = virt_cast<UCReadSysConfigRequest *>(vecs[0].vaddr);

            for (auto i = 0u; i < count; ++i) {
               settings[i].error = request->settings[i].error;

               if (settings[i].error) {
                  result = settings[i].error;
               }

               if (!settings[i].data) {
                  result = UCError::InvalidParam;
               }

               if (settings[i].error == UCError::OK) {
                  auto src = virt_cast<void *>(vecs[i + 1].vaddr);

                  switch (settings[i].dataSize) {
                  case 0:
                     continue;
                  case 1:
                     *virt_cast<uint8_t *>(settings[i].data) = *virt_cast<uint8_t *>(src);
                     break;
                  case 2:
                     *virt_cast<uint16_t *>(settings[i].data) = *virt_cast<uint16_t *>(src);
                     break;
                  case 4:
                     *virt_cast<uint32_t *>(settings[i].data) = *virt_cast<uint32_t *>(src);
                     break;
                  default:
                     std::memset(settings[i].data.getRawPointer(), 0, 4); // why???
                     std::memcpy(settings[i].data.getRawPointer(), src.getRawPointer(), settings[i].dataSize);
                  }
               }
            }
         } else if (command == UCCommand::WriteSysConfig) {
            auto request = virt_cast<UCWriteSysConfigRequest *>(vecs[0].vaddr);

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
         cafe::invoke(cpu::this_core::state(),
                      callback,
                      result,
                      command,
                      count,
                      settings,
                      callbackContext);
      }
   }

   if (vecs) {
      for (auto i = 0u; i < count + 1; ++i) {
         internal::ucFreeMessage(virt_cast<void *>(vecs[i].vaddr));
      }

      internal::ucFreeMessage(vecs);
   }

   return static_cast<UCError>(result);
}


static void
ucIosAsyncCallback(IOSError status,
                   virt_ptr<void> context)
{
   auto asyncParams = virt_cast<UCAsyncParams *>(context);
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
   OSInitMutex(virt_addrof(sUserConfigData->lock));
   OSLockMutex(virt_addrof(sUserConfigData->lock));

   if (!sUserConfigData->initialised) {
      if (!sUserConfigData->smallMessagePool) {
         sUserConfigData->smallMessagePool = IPCBufPoolCreate(virt_addrof(sUserConfigData->smallMessageBuffer),
                                                             static_cast<uint32_t>(sUserConfigData->smallMessageBuffer.size()),
                                                             SmallMessageSize,
                                                             virt_addrof(sUserConfigData->smallMessageCount),
                                                             1);
      }

      if (!sUserConfigData->largeMessagePool) {
         sUserConfigData->largeMessagePool = IPCBufPoolCreate(virt_addrof(sUserConfigData->largeMessageBuffer),
                                                             static_cast<uint32_t>(sUserConfigData->largeMessageBuffer.size()),
                                                             LargeMessageSize,
                                                             virt_addrof(sUserConfigData->largeMessageCount),
                                                             1);
      }

      if (sUserConfigData->smallMessagePool && sUserConfigData->largeMessagePool) {
         sUserConfigData->initialised = true;
      }
   }

   OSUnlockMutex(virt_addrof(sUserConfigData->lock));

   if (!sUserConfigData->initialised) {
      return UCError::Error;
   }

   return static_cast<UCError>(IOS_Open(make_stack_string("/dev/usr_cfg"),
                                        IOSOpenMode::None));
}


UCError
UCClose(IOSHandle handle)
{
   return static_cast<UCError>(IOS_Close(handle));
}


UCError
UCDeleteSysConfig(IOSHandle handle,
                  uint32_t count,
                  virt_ptr<UCSysConfig> settings)
{
   return UCError::OK;
}


UCError
UCReadSysConfig(IOSHandle handle,
                uint32_t count,
                virt_ptr<UCSysConfig> settings)
{
   return UCReadSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCReadSysConfigAsync(IOSHandle handle,
                     uint32_t count,
                     virt_ptr<UCSysConfig> settings,
                     virt_ptr<UCAsyncParams> asyncParams)
{
   auto result = UCError::OK;
   uint32_t msgBufSize = 0, vecBufSize = 0;
   virt_ptr<void> msgBuf = nullptr, vecBuf = nullptr;
   virt_ptr<UCReadSysConfigRequest> request = nullptr;
   virt_ptr<IOSVec> vecs = nullptr;

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

   request = virt_cast<UCReadSysConfigRequest *>(msgBuf);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings,
               settings.getRawPointer(),
               sizeof(UCSysConfig) * count);

   vecBufSize = static_cast<uint32_t>((count + 1) * sizeof(IOSVec));
   vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   vecs = virt_cast<IOSVec *>(vecBuf);
   vecs[0].vaddr = virt_cast<virt_addr>(msgBuf);
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].vaddr = virt_cast<virt_addr>(internal::ucAllocateMessage(size));
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
                                                    sUcIosAsyncCallback,
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
            internal::ucFreeMessage(virt_cast<void *>(vecs[1 + i].vaddr));
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
                 virt_ptr<UCSysConfig> settings)
{
   return UCWriteSysConfigAsync(handle, count, settings, nullptr);
}


UCError
UCWriteSysConfigAsync(IOSHandle handle,
                      uint32_t count,
                      virt_ptr<UCSysConfig> settings,
                      virt_ptr<UCAsyncParams> asyncParams)
{
   auto result = UCError::OK;
   uint32_t msgBufSize = 0, vecBufSize = 0;
   virt_ptr<void> vecBuf = nullptr, msgBuf = nullptr;
   virt_ptr<UCWriteSysConfigRequest> request = nullptr;
   virt_ptr<IOSVec> vecs = nullptr;

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

   request = virt_cast<UCWriteSysConfigRequest *>(msgBuf);
   request->unk0x00 = 0u;
   request->count = count;
   std::memcpy(request->settings,
               settings.getRawPointer(),
               count * sizeof(UCSysConfig));

   vecBufSize = static_cast<uint32_t>((count + 1) * sizeof(IOSVec));
   vecBuf = internal::ucAllocateMessage(vecBufSize);
   if (!vecBuf) {
      result = UCError::NoIPCBuffers;
      goto fail;
   }

   vecs = virt_cast<IOSVec *>(vecBuf);
   vecs[0].vaddr = virt_cast<virt_addr>(msgBuf);
   vecs[0].len = msgBufSize;

   for (auto i = 0u; i < count; ++i) {
      auto size = settings[i].dataSize;
      vecs[1 + i].len = size;

      if (size > 0) {
         vecs[1 + i].vaddr = virt_cast<virt_addr>(internal::ucAllocateMessage(size));
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
                                                    sUcIosAsyncCallback,
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
            internal::ucFreeMessage(virt_cast<void *>(vecs[1 + i].vaddr));
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
Library::registerUserConfigSymbols()
{
   RegisterFunctionExport(UCOpen);
   RegisterFunctionExport(UCClose);
   RegisterFunctionExport(UCReadSysConfig);
   RegisterFunctionExport(UCReadSysConfigAsync);
   RegisterFunctionExport(UCWriteSysConfig);
   RegisterFunctionExport(UCWriteSysConfigAsync);

   RegisterDataInternal(sUserConfigData);
   RegisterFunctionInternal(internal::ucIosAsyncCallback, sUcIosAsyncCallback);
}

} // namespace cafe::coreinit
