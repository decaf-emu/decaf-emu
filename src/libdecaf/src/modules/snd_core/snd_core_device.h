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
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            be_AXDeviceFinalMixCallback *func);

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type,
                                 AXDeviceFinalMixCallback func);

AXResult
AXGetAuxCallback(AXDeviceType type,
                 uint32_t,
                 uint32_t,
                 be_AXAuxCallback *callback,
                 be_ptr<void> *userData);

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t,
                      uint32_t,
                      AXAuxCallback callback,
                      void *userData);

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type,
                           uint32_t,
                           uint32_t);

AXResult
AXSetDeviceCompressor(AXDeviceType type,
                      uint32_t);

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type,
                         BOOL postFinalMix);

AXResult
AXSetDeviceVolume(AXDeviceType type,
                  uint32_t id,
                  uint16_t volume);

namespace internal
{

void
mixOutput(int32_t *buffer, int numSamples, int numChannels);

} // namespace internal

} // namespace snd_core
