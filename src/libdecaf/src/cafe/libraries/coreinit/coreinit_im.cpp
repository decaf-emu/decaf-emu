#include "coreinit.h"
#include "coreinit_im.h"
#include "coreinit_ios.h"
#include "coreinit_mutex.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"
#include <libcpu/cpu.h>

namespace cafe::coreinit
{

struct StaticImData
{
   be2_struct<OSMutex> itbMutex;
   be2_array<char, 16> itbMutexName;
   be2_struct<IMRequest> sharedRequest;
};

static virt_ptr<StaticImData> sImData = nullptr;
static IOSAsyncCallbackFn sImIosAsyncCallback = nullptr;

namespace internal
{

static void
imAcquireItbMutex()
{
   OSLockMutex(virt_addrof(sImData->itbMutex));
}

static void
imReleaseItbMutex()
{
   OSUnlockMutex(virt_addrof(sImData->itbMutex));
}

static void
imCopyData(virt_ptr<IMRequest> request)
{
   if (request->copyDst && request->copySrc && request->copySize) {
      std::memcpy(request->copyDst.get(),
                  request->copySrc.get(),
                  request->copySize);
   }
}

static void
imIosAsyncCallback(IOSError error,
                   virt_ptr<void> context)
{
   auto request = virt_cast<IMRequest *>(context);
   if (error == IOSError::OK) {
      imCopyData(request);
   }

   cafe::invoke(cpu::this_core::state(),
                request->asyncCallback,
                error,
                request->asyncCallbackContext);
}

static IMError
imSendRequest(virt_ptr<IMRequest> request,
              uint32_t vecIn,
              uint32_t vecOut)
{
   auto error = IOSError::OK;

   if (request->asyncCallback) {
      error = IOS_IoctlvAsync(request->handle,
                              request->request,
                              vecIn,
                              vecOut,
                              virt_addrof(request->ioctlVecs),
                              sImIosAsyncCallback,
                              request);
   } else {
      error = IOS_Ioctlv(request->handle,
                         request->request,
                         vecIn,
                         vecOut,
                         virt_addrof(request->ioctlVecs));

      if (vecOut > 0 && error == IOSError::OK) {
         imCopyData(request);
      }
   }

   return static_cast<IMError>(error);
}

} // namespace internal


IMError
IM_Open()
{
   return static_cast<IMError>(IOS_Open(cafe::make_stack_string("/dev/im"),
                                        IOSOpenMode::None));
}


IMError
IM_Close(IOSHandle handle)
{
   return static_cast<IMError>(IOS_Close(handle));
}


IMError
IM_GetHomeButtonParams(IOSHandle handle,
                       virt_ptr<IMRequest> request,
                       virt_ptr<void> output,
                       IOSAsyncCallbackFn asyncCallback,
                       virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->getHomeButtomParamResponse));
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetHomeButtonParams;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;
   request->copySrc = virt_addrof(request->getHomeButtomParamResponse);
   request->copyDst = output;
   request->copySize = 8u;

   return internal::imSendRequest(request, 0, 1);
}


IMError
IM_GetParameter(IOSHandle handle,
                virt_ptr<IMRequest> request,
                IMParameter parameter,
                virt_ptr<void> output,
                IOSAsyncCallbackFn asyncCallback,
                virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->getParameterRequest.parameter = parameter;
   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->getParameterRequest));
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_cast<virt_addr>(virt_addrof(request->getParameterResponse));
   request->ioctlVecs[1].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;
   request->copySrc = virt_addrof(request->getParameterResponse.value);
   request->copyDst = output;
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetParameters(virt_ptr<IMParameters> parameters)
{
   auto result = IM_Open();
   if (result < 0) {
      return result;
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            IMParameter::ResetEnable,
                            virt_addrof(parameters->resetEnabled),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            IMParameter::DimEnabled,
                            virt_addrof(parameters->dimEnabled),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            IMParameter::DimPeriod,
                            virt_addrof(parameters->dimPeriod),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            IMParameter::APDEnabled,
                            virt_addrof(parameters->apdEnabled),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            IMParameter::APDPeriod,
                            virt_addrof(parameters->apdPeriod),
                            nullptr,
                            nullptr);
   if (result != IMError::OK) {
      goto out;
   }

out:
   internal::imReleaseItbMutex();
   IM_Close(handle);
   return result;
}


IMError
IM_GetNvParameter(IOSHandle handle,
                  virt_ptr<IMRequest> request,
                  IMParameter parameter,
                  virt_ptr<void> output,
                  IOSAsyncCallbackFn asyncCallback,
                  virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->getNvParameterRequest.parameter = parameter;
   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->getNvParameterRequest));
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_cast<virt_addr>(virt_addrof(request->getNvParameterResponse));
   request->ioctlVecs[1].len = 8u;

   request->handle = handle;
   request->request = IMCommand::GetNvParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;
   request->copySrc = virt_addrof(request->getNvParameterResponse.value);
   request->copyDst = output;
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetNvParameterWithoutHandleAndItb(IMParameter parameter,
                                     virt_ptr<uint32_t> outValue)
{
   auto result = IM_Open();
   if (result < 0) {
      return result;
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_GetNvParameter(handle,
                              virt_addrof(sImData->sharedRequest),
                              parameter,
                              outValue,
                              nullptr,
                              nullptr);

   internal::imReleaseItbMutex();
   IM_Close(handle);
   return result;
}


IMError
IM_GetRuntimeParameter(IMParameter parameter,
                       virt_ptr<uint32_t> outValue)
{
   auto result = IM_Open();
   if (result < 0) {
      return result;
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_GetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            parameter,
                            outValue,
                            nullptr,
                            nullptr);

   internal::imReleaseItbMutex();
   IM_Close(handle);
   return result;
}


IMError
IM_GetTimerRemaining(IOSHandle handle,
                     virt_ptr<IMRequest> request,
                     IMTimer timer,
                     virt_ptr<void> output,
                     IOSAsyncCallbackFn asyncCallback,
                     virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->getTimerRemainingRequest.timer = timer;
   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->getTimerRemainingRequest));
   request->ioctlVecs[0].len = 8u;

   request->ioctlVecs[1].vaddr = virt_cast<virt_addr>(virt_addrof(request->getTimerRemainingResponse));
   request->ioctlVecs[1].len = static_cast<uint32_t>(sizeof(IMGetTimerRemainingResponse));

   request->handle = handle;
   request->request = IMCommand::GetTimerRemaining;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;
   request->copySrc = virt_addrof(request->getTimerRemainingResponse.value);
   request->copyDst = output;
   request->copySize = 4u;

   return internal::imSendRequest(request, 1, 1);
}


IMError
IM_GetTimerRemainingSeconds(IMTimer timer,
                            virt_ptr<uint32_t> outSeconds)
{
   auto result = IM_Open();
   if (result < 0) {
      return result;
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_GetTimerRemaining(handle,
                                 virt_addrof(sImData->sharedRequest),
                                 timer,
                                 outSeconds,
                                 nullptr,
                                 nullptr);

   internal::imReleaseItbMutex();
   IM_Close(handle);
   return result;
}


IMError
IM_SetParameter(IOSHandle handle,
                virt_ptr<IMRequest> request,
                IMParameter parameter,
                uint32_t value,
                IOSAsyncCallbackFn asyncCallback,
                virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->setParameterRequest.parameter = parameter;
   request->setParameterRequest.value = value;
   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->setParameterRequest));
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::SetParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;

   return internal::imSendRequest(request, 1, 0);
}


IMError
IM_SetNvParameter(IOSHandle handle,
                  virt_ptr<IMRequest> request,
                  IMParameter parameter,
                  uint32_t value,
                  IOSAsyncCallbackFn asyncCallback,
                  virt_ptr<void> asyncCallbackContext)
{
   std::memset(request.get(), 0, sizeof(IMRequest));

   request->setNvParameterRequest.parameter = parameter;
   request->setNvParameterRequest.value = value;
   request->ioctlVecs[0].vaddr = virt_cast<virt_addr>(virt_addrof(request->setNvParameterRequest));
   request->ioctlVecs[0].len = 8u;

   request->handle = handle;
   request->request = IMCommand::SetNvParameter;
   request->asyncCallback = asyncCallback;
   request->asyncCallbackContext = asyncCallbackContext;

   return internal::imSendRequest(request, 1, 0);
}


IMError
IM_SetRuntimeParameter(IMParameter parameter,
                       uint32_t value)
{
   auto result = IM_Open();
   if (result < 0) {
      return result;
   }

   auto handle = static_cast<IOSHandle>(result);
   internal::imAcquireItbMutex();

   result = IM_SetParameter(handle,
                            virt_addrof(sImData->sharedRequest),
                            parameter,
                            value,
                            nullptr,
                            nullptr);

   internal::imReleaseItbMutex();
   IM_Close(handle);
   return result;
}


IMError
IMDisableAPD()
{
   return IM_SetRuntimeParameter(IMParameter::APDEnabled, FALSE);
}


IMError
IMDisableDim()
{
   auto result = IM_SetRuntimeParameter(IMParameter::DimEnabled, FALSE);
   if (result != IMError::OK) {
      return result;
   }

   return IM_SetRuntimeParameter(IMParameter::ResetEnable, FALSE);
}


IMError
IMEnableAPD()
{
   auto prevValue = StackObject<uint32_t> { };
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDEnabled,
                                                      prevValue);

   if (result != IMError::OK) {
      return result;
   }

   if (*prevValue == TRUE) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::APDEnabled, TRUE);
}


IMError
IMEnableDim()
{
   auto prevValue = StackObject<uint32_t> { };
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::DimEnabled,
                                                      prevValue);
   if (result != IMError::OK) {
      return result;
   }

   if (*prevValue == TRUE) {
      return IMError::OK;
   }

   result = IM_SetRuntimeParameter(IMParameter::DimEnabled, TRUE);
   if (result != IMError::OK) {
      return result;
   }

   result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::ResetEnable,
                                                 prevValue);
   if (result != IMError::OK) {
      return result;
   }

   if (*prevValue == TRUE) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::ResetEnable, FALSE);
}


IMError
IMIsAPDEnabled(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::APDEnabled, outValue);
}


IMError
IMIsAPDEnabledBySysSettings(virt_ptr<uint32_t> outValue)
{
   return IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDEnabled,
                                               outValue);
}


IMError
IMIsDimEnabled(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnabled, outValue);
}


IMError
IMGetAPDPeriod(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::APDPeriod, outValue);
}


IMError
IMGetDimEnableDrc(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnableDrc, outValue);
}


IMError
IMGetDimEnableTv(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::DimEnableTv, outValue);
}


IMError
IMGetDimPeriod(virt_ptr<uint32_t> outValue)
{
   return IM_GetRuntimeParameter(IMParameter::DimPeriod, outValue);
}


IMError
IMGetTimeBeforeAPD(virt_ptr<uint32_t> outSeconds)
{
   return IM_GetTimerRemainingSeconds(IMTimer::APD, outSeconds);
}


IMError
IMGetTimeBeforeDimming(virt_ptr<uint32_t> outSeconds)
{
   return IM_GetTimerRemainingSeconds(IMTimer::Dim, outSeconds);
}


IMError
IMSetDimEnableDrc(BOOL value)
{
   return IM_SetRuntimeParameter(IMParameter::DimEnableTv, value);
}


IMError
IMSetDimEnableTv(BOOL value)
{
   return IM_SetRuntimeParameter(IMParameter::DimEnableTv, value);
}


IMError
IMStartAPDVideoMode()
{
   auto prevValue = StackObject<uint32_t> { };
   auto result = IM_GetNvParameterWithoutHandleAndItb(IMParameter::APDPeriod,
                                                      prevValue);

   if (result != IMError::OK) {
      return result;
   }

   if (*prevValue == 14400) {
      return IMError::OK;
   }

   return IM_SetRuntimeParameter(IMParameter::APDPeriod, 14400);
}

namespace internal
{

void
initialiseIm()
{
   sImData->itbMutexName = "itb_mutex";
   OSInitMutexEx(virt_addrof(sImData->itbMutex),
                 virt_addrof(sImData->itbMutexName));
}

} // namespace internal

void
Library::registerImSymbols()
{
   RegisterFunctionExport(IM_Open);
   RegisterFunctionExport(IM_Close);
   RegisterFunctionExport(IM_GetHomeButtonParams);
   RegisterFunctionExport(IM_GetParameter);
   RegisterFunctionExport(IM_GetParameters);
   RegisterFunctionExport(IM_GetNvParameter);
   RegisterFunctionExport(IM_GetNvParameterWithoutHandleAndItb);
   RegisterFunctionExport(IM_GetRuntimeParameter);
   RegisterFunctionExport(IM_GetTimerRemaining);
   RegisterFunctionExport(IM_GetTimerRemainingSeconds);
   RegisterFunctionExport(IM_SetParameter);
   RegisterFunctionExport(IM_SetRuntimeParameter);

   RegisterFunctionExport(IMDisableAPD);
   RegisterFunctionExport(IMDisableDim);
   RegisterFunctionExport(IMEnableAPD);
   RegisterFunctionExport(IMEnableDim);
   RegisterFunctionExport(IMIsAPDEnabled);
   RegisterFunctionExport(IMIsAPDEnabledBySysSettings);
   RegisterFunctionExport(IMIsDimEnabled);
   RegisterFunctionExport(IMGetAPDPeriod);
   RegisterFunctionExport(IMGetDimEnableDrc);
   RegisterFunctionExport(IMGetDimEnableTv);
   RegisterFunctionExport(IMGetDimPeriod);
   RegisterFunctionExport(IMGetTimeBeforeAPD);
   RegisterFunctionExport(IMGetTimeBeforeDimming);
   RegisterFunctionExport(IMSetDimEnableDrc);
   RegisterFunctionExport(IMSetDimEnableTv);
   RegisterFunctionExport(IMStartAPDVideoMode);

   RegisterDataInternal(sImData);
   RegisterFunctionInternal(internal::imIosAsyncCallback, sImIosAsyncCallback);
}

} // namespace cafe::coreinit
