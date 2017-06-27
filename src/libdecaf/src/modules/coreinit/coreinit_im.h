#pragma once
#include "coreinit_ios.h"
#include "ios/dev/im.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/cbool.h>
#include <common/structsize.h>

namespace coreinit
{

/**
 * \defgroup coreinit_im IM
 * \ingroup coreinit
 * @{
 */

using ios::dev::im::IMCommand;
using ios::dev::im::IMError;
using ios::dev::im::IMParameter;
using ios::dev::im::IMTimer;

using ios::dev::im::IMGetNvParameterRequest;
using ios::dev::im::IMGetNvParameterResponse;
using ios::dev::im::IMGetParameterRequest;
using ios::dev::im::IMGetParameterResponse;
using ios::dev::im::IMGetHomeButtonParamResponse;
using ios::dev::im::IMSetParameterRequest;
using ios::dev::im::IMSetNvParameterRequest;
using ios::dev::im::IMGetTimerRemainingRequest;
using ios::dev::im::IMGetTimerRemainingResponse;

struct IMParameters
{
   be_val<uint32_t> unknown0x00;
   be_val<uint32_t> dimEnabled;
   be_val<uint32_t> dimPeriod;
   be_val<uint32_t> apdEnabled;
   be_val<uint32_t> apdPeriod;
};
CHECK_OFFSET(IMParameters, 0x00, unknown0x00);
CHECK_OFFSET(IMParameters, 0x04, dimEnabled);
CHECK_OFFSET(IMParameters, 0x08, dimPeriod);
CHECK_OFFSET(IMParameters, 0x0C, apdEnabled);
CHECK_OFFSET(IMParameters, 0x10, apdPeriod);

struct IMRequest
{
   union
   {
      IMGetNvParameterRequest getNvParameterRequest;
      IMGetNvParameterResponse getNvParameterResponse;
      IMGetParameterRequest getParameterRequest;
      IMGetParameterResponse getParameterResponse;
      IMGetHomeButtonParamResponse getHomeButtomParamResponse;
      IMSetParameterRequest setParameterRequest;
      IMSetNvParameterRequest setNvParameterRequest;
      IMGetTimerRemainingRequest getTimerRemainingRequest;
      IMGetTimerRemainingResponse getTimerRemainingResponse;
      uint8_t args[0x80];
   };

   IOSVec ioctlVecs[2];
   be_val<IOSHandle> handle;
   be_val<IMCommand> request;
   IOSAsyncCallbackFn asyncCallback;
   be_ptr<void> asyncCallbackContext;
   be_ptr<void> copySrc;
   be_ptr<void> copyDst;
   be_val<uint32_t> copySize;
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
