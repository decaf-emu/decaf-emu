#pragma once
#include "coreinit_ios.h"
#include "ios/auxil/ios_auxil_im.h"

#include <cstdint>
#include <common/cbool.h>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace coreinit
{

/**
 * \defgroup coreinit_im IM
 * \ingroup coreinit
 * @{
 */

using IMError = ios::Error;

using ios::auxil::IMCommand;
using ios::auxil::IMParameter;
using ios::auxil::IMTimer;

using ios::auxil::IMGetNvParameterRequest;
using ios::auxil::IMGetNvParameterResponse;
using ios::auxil::IMGetParameterRequest;
using ios::auxil::IMGetParameterResponse;
using ios::auxil::IMGetHomeButtonParamResponse;
using ios::auxil::IMSetParameterRequest;
using ios::auxil::IMSetNvParameterRequest;
using ios::auxil::IMGetTimerRemainingRequest;
using ios::auxil::IMGetTimerRemainingResponse;

struct IMParameters
{
   be2_val<uint32_t> resetEnabled;
   be2_val<uint32_t> dimEnabled;
   be2_val<uint32_t> dimPeriod;
   be2_val<uint32_t> apdEnabled;
   be2_val<uint32_t> apdPeriod;
};
CHECK_OFFSET(IMParameters, 0x00, resetEnabled);
CHECK_OFFSET(IMParameters, 0x04, dimEnabled);
CHECK_OFFSET(IMParameters, 0x08, dimPeriod);
CHECK_OFFSET(IMParameters, 0x0C, apdEnabled);
CHECK_OFFSET(IMParameters, 0x10, apdPeriod);

struct IMRequest
{
   union
   {
      be2_struct<IMGetNvParameterRequest> getNvParameterRequest;
      be2_struct<IMGetNvParameterResponse> getNvParameterResponse;
      be2_struct<IMGetParameterRequest> getParameterRequest;
      be2_struct<IMGetParameterResponse> getParameterResponse;
      be2_struct<IMGetHomeButtonParamResponse> getHomeButtomParamResponse;
      be2_struct<IMSetParameterRequest> setParameterRequest;
      be2_struct<IMSetNvParameterRequest> setNvParameterRequest;
      be2_struct<IMGetTimerRemainingRequest> getTimerRemainingRequest;
      be2_struct<IMGetTimerRemainingResponse> getTimerRemainingResponse;
      be2_array<uint8_t, 0x80> args;
   };

   be2_array<IOSVec, 2> ioctlVecs;
   be2_val<IOSHandle> handle;
   be2_val<IMCommand> request;
   IOSAsyncCallbackFn::be asyncCallback;
   be2_virt_ptr<void> asyncCallbackContext;
   be2_virt_ptr<void> copySrc;
   be2_virt_ptr<void> copyDst;
   be2_val<uint32_t> copySize;
};
CHECK_OFFSET(IMRequest, 0x80, ioctlVecs);
CHECK_OFFSET(IMRequest, 0x98, handle);
CHECK_OFFSET(IMRequest, 0x9C, request);
CHECK_OFFSET(IMRequest, 0xA0, asyncCallback);
CHECK_OFFSET(IMRequest, 0xA4, asyncCallbackContext);
CHECK_OFFSET(IMRequest, 0xA8, copySrc);
CHECK_OFFSET(IMRequest, 0xAC, copyDst);
CHECK_OFFSET(IMRequest, 0xB0, copySize);

IMError
IM_Open();

IMError
IM_Close(IOSHandle handle);

IMError
IM_GetHomeButtonParams(IOSHandle handle,
                       IMRequest *request,
                       void *output,
                       IOSAsyncCallbackFn asyncCallback,
                       void *asyncCallbackContext);

IMError
IM_GetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                void *output,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext);

IMError
IM_GetParameters(IMParameters *parameters);

IMError
IM_GetNvParameter(IOSHandle handle,
                  IMRequest *request,
                  IMParameter parameter,
                  void *output,
                  IOSAsyncCallbackFn asyncCallback,
                  void *asyncCallbackContext);

IMError
IM_GetNvParameterWithoutHandleAndItb(IMParameter parameter,
                                     be_val<uint32_t> *value);

IMError
IM_GetRuntimeParameter(IMParameter parameter,
                       be_val<uint32_t> *value);

IMError
IM_GetTimerRemaining(IOSHandle handle,
                     IMRequest *request,
                     IMTimer timer,
                     void *output,
                     IOSAsyncCallbackFn asyncCallback,
                     void *asyncCallbackContext);

IMError
IM_GetTimerRemainingSeconds(IMTimer timer,
                            be_val<uint32_t> *seconds);

IMError
IM_SetParameter(IOSHandle handle,
                IMRequest *request,
                IMParameter parameter,
                uint32_t value,
                IOSAsyncCallbackFn asyncCallback,
                void *asyncCallbackContext);

IMError
IM_SetRuntimeParameter(IMParameter parameter,
                       uint32_t value);

IMError
IMDisableAPD();

IMError
IMDisableDim();

IMError
IMEnableAPD();

IMError
IMEnableDim();

IMError
IMIsAPDEnabled(be_val<uint32_t> *value);

IMError
IMIsAPDEnabledBySysSettings(be_val<uint32_t> *value);

IMError
IMIsDimEnabled(be_val<uint32_t> *value);

IMError
IMGetDimEnableDrc(be_val<uint32_t> *value);

IMError
IMGetDimEnableTv(be_val<uint32_t> *value);

IMError
IMGetDimPeriod(be_val<uint32_t> *value);

IMError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds);

IMError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds);

IMError
IMSetDimEnableDrc(BOOL value);

IMError
IMSetDimEnableTv(BOOL value);

IMError
IMStartAPDVideoMode();

/** @} */

} // namespace coreinit
