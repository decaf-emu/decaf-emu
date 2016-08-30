#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "ppcutils/wfunc_ptr.h"
#include "snd_core_enum.h"

namespace snd_core
{

using AXDeviceFinalMixCallback = wfunc_ptr<void>;
using be_AXDeviceFinalMixCallback = be_wfunc_ptr<void>;
using AXAuxCallback = wfunc_ptr<void>;
using be_AXAuxCallback = wfunc_ptr<void>;

AXResult
AXGetDeviceMode(AXDeviceType type,
                be_val<AXDeviceMode> *mode);

AXResult
AXSetDeviceMode(AXDeviceType type,
                AXDeviceMode mode);

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            be_AXDeviceFinalMixCallback *func);

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type,
                                 AXDeviceFinalMixCallback func);

AXResult
AXGetAuxCallback(AXDeviceType type,
                 uint32_t deviceId,
                 AXAuxId auxId,
                 be_AXAuxCallback *callback,
                 be_ptr<void> *userData);

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t deviceId,
                      AXAuxId auxId,
                      AXAuxCallback callback,
                      void *userData);

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type,
                           uint32_t deviceId,
                           BOOL linear);

AXResult
AXSetDeviceCompressor(AXDeviceType type,
                      BOOL compressor);

AXResult
AXGetDeviceUpsampleStage(AXDeviceType type,
                         BOOL *upsampleAfterFinalMixCallback);

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type,
                         BOOL upsampleAfterFinalMixCallback);

AXResult
AXGetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  be_val<uint16_t> *volume);

AXResult
AXSetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  uint16_t volume);

AXResult
AXGetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     be_val<uint16_t> *volume);

AXResult
AXSetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     uint16_t volume);

namespace internal
{

void
mixOutput(int32_t *buffer, int numSamples, int numChannels);

} // namespace internal

} // namespace snd_core
