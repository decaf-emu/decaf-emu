#pragma once
#include "ppcutils/wfunc_ptr.h"
#include "snd_core_enum.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/cbool.h>
#include <cstdint>

namespace snd_core
{

#pragma pack(push, 1)

struct AXAuxCallbackData
{
   be_val<uint32_t> channels;
   be_val<uint32_t> samples;
};
CHECK_OFFSET(AXAuxCallbackData, 0x0, channels);
CHECK_OFFSET(AXAuxCallbackData, 0x4, samples);
CHECK_SIZE(AXAuxCallbackData, 0x8);

struct AXDeviceFinalMixData
{
   be_ptr<be_ptr<be_val<int32_t>>> data;
   be_val<uint16_t> channels;
   be_val<uint16_t> samples;
   be_val<uint16_t> numDevices;
   be_val<uint16_t> channelsOut;
};
CHECK_OFFSET(AXDeviceFinalMixData, 0x0, data);
CHECK_OFFSET(AXDeviceFinalMixData, 0x4, channels);
CHECK_OFFSET(AXDeviceFinalMixData, 0x6, samples);
CHECK_OFFSET(AXDeviceFinalMixData, 0x8, numDevices);
CHECK_OFFSET(AXDeviceFinalMixData, 0xa, channelsOut);
CHECK_SIZE(AXDeviceFinalMixData, 0xc);

#pragma pack(pop)

using AXDeviceFinalMixCallback = wfunc_ptr<void, AXDeviceFinalMixData*>;
using be_AXDeviceFinalMixCallback = be_wfunc_ptr<void, AXDeviceFinalMixData*>;
using AXAuxCallback = wfunc_ptr<void, be_ptr<be_val<int32_t>>*, void *, AXAuxCallbackData *>;
using be_AXAuxCallback = wfunc_ptr<void, be_ptr<be_val<int32_t>>*, void *, AXAuxCallbackData *>;

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
mixOutput(int32_t *buffer,
          int numSamples,
          int numChannels);

} // namespace internal

} // namespace snd_core
