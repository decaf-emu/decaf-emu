#pragma once
#include "sndcore2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::sndcore2
{

#pragma pack(push, 1)

struct AXAuxCallbackData
{
   be2_val<uint32_t> channels;
   be2_val<uint32_t> samples;
};
CHECK_OFFSET(AXAuxCallbackData, 0x0, channels);
CHECK_OFFSET(AXAuxCallbackData, 0x4, samples);
CHECK_SIZE(AXAuxCallbackData, 0x8);

struct AXDeviceFinalMixData
{
   be2_virt_ptr<virt_ptr<int32_t>> data;
   be2_val<uint16_t> channels;
   be2_val<uint16_t> samples;
   be2_val<uint16_t> numDevices;
   be2_val<uint16_t> channelsOut;
};
CHECK_OFFSET(AXDeviceFinalMixData, 0x0, data);
CHECK_OFFSET(AXDeviceFinalMixData, 0x4, channels);
CHECK_OFFSET(AXDeviceFinalMixData, 0x6, samples);
CHECK_OFFSET(AXDeviceFinalMixData, 0x8, numDevices);
CHECK_OFFSET(AXDeviceFinalMixData, 0xa, channelsOut);
CHECK_SIZE(AXDeviceFinalMixData, 0xc);

#pragma pack(pop)

using AXDeviceFinalMixCallback = virt_func_ptr<
   void(virt_ptr<AXDeviceFinalMixData>)
>;

using AXAuxCallback = virt_func_ptr<
   void(virt_ptr<virt_ptr<int32_t>>, virt_ptr<void>, virt_ptr<AXAuxCallbackData>)
>;

AXResult
AXGetDeviceMode(AXDeviceType type,
                virt_ptr<AXDeviceMode> mode);

AXResult
AXSetDeviceMode(AXDeviceType type,
                AXDeviceMode mode);

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            virt_ptr<AXDeviceFinalMixCallback> outCallback);

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type,
                                 AXDeviceFinalMixCallback callback);

AXResult
AXGetAuxCallback(AXDeviceType type,
                 uint32_t deviceId,
                 AXAuxId auxId,
                 virt_ptr<AXAuxCallback> outCallback,
                 virt_ptr<virt_ptr<void>> outUserData);

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t deviceId,
                      AXAuxId auxId,
                      AXAuxCallback callback,
                      virt_ptr<void> userData);

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type,
                           uint32_t deviceId,
                           BOOL linear);

AXResult
AXSetDeviceCompressor(AXDeviceType type,
                      BOOL compressor);

AXResult
AXGetDeviceUpsampleStage(AXDeviceType type,
                         virt_ptr<BOOL> outUpsampleAfterFinalMixCallback);

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type,
                         BOOL upsampleAfterFinalMixCallback);

AXResult
AXGetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  virt_ptr<uint16_t> outVolume);

AXResult
AXSetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  uint16_t volume);

AXResult
AXGetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     virt_ptr<uint16_t> outVolume);

AXResult
AXSetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     uint16_t volume);

namespace internal
{

void
mixOutput(virt_ptr<int32_t> buffer,
          int numSamples,
          int numChannels);

} // namespace internal

} // namespace cafe::sndcore2
