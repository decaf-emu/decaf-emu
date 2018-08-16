#pragma once
#include "sndcore2_constants.h"
#include "sndcore2_device.h"
#include "sndcore2_enum.h"
#include "cafe/cafe_ppc_interface.h"

#include <common/fixed.h>
#include <cstdint>
#include <libcpu/be2_struct.h>
#include <vector>

namespace cafe::sndcore2
{

using Pcm16Sample = sg14::make_fixed<15, 0, int16_t>;

using AXVoiceCallbackFn = virt_func_ptr<
   virt_ptr<void>()
>;
using AXVoiceCallbackExFn = virt_func_ptr<
   virt_ptr<void>(uint32_t, uint32_t)
>;

#pragma pack(push, 1)

struct AXVoice;

struct AXVoiceLink
{
   be2_virt_ptr<AXVoice> next;
   be2_virt_ptr<AXVoice> prev;
};
CHECK_OFFSET(AXVoiceLink, 0x0, next);
CHECK_OFFSET(AXVoiceLink, 0x4, prev);
CHECK_SIZE(AXVoiceLink, 0x8);

struct AXVoiceOffsets
{
   be2_val<AXVoiceFormat> dataType;
   be2_val<AXVoiceLoop> loopingEnabled;
   be2_val<uint32_t> loopOffset;
   be2_val<uint32_t> endOffset;
   be2_val<uint32_t> currentOffset;
   be2_virt_ptr<const void> data;
};
CHECK_OFFSET(AXVoiceOffsets, 0x0, dataType);
CHECK_OFFSET(AXVoiceOffsets, 0x2, loopingEnabled);
CHECK_OFFSET(AXVoiceOffsets, 0x4, loopOffset);
CHECK_OFFSET(AXVoiceOffsets, 0x8, endOffset);
CHECK_OFFSET(AXVoiceOffsets, 0xc, currentOffset);
CHECK_OFFSET(AXVoiceOffsets, 0x10, data);
CHECK_SIZE(AXVoiceOffsets, 0x14);

struct AXVoice
{
   //! The index of this voice out of the total voices
   be2_val<uint32_t> index;

   //! Current play state of this voice
   be2_val<AXVoiceState> state;

   //! Current volume of this voice
   be2_val<uint32_t> volume;

   //! The renderer to use for this voice
   be2_val<AXRenderer> renderer;

   //! this is a link used in the stack, we do this in host-memory currently
   be2_struct<AXVoiceLink> link;

   //! A link to the next callback to invoke
   be2_virt_ptr<AXVoice> cbNext;

   //! The priority of this voice used for force-acquiring a voice
   be2_val<uint32_t> priority;

   //! The callback to call if this is force-free'd by another acquire
   be2_val<AXVoiceCallbackFn> callback;

   //! The user context to send to the callbacks
   be2_virt_ptr<void> userContext;

   //! A bitfield representing different things needing to be synced.
   be2_val<uint32_t> syncBits;

   UNKNOWN(0x8);

   //! The current offset data!
   be2_struct<AXVoiceOffsets> offsets;

   //! An extended version of the callback above
   be2_val<AXVoiceCallbackExFn> callbackEx;

   //! The reason for the callback being invoked
   be2_val<uint32_t> callbackReason;

   be2_val<float> unk0;
   be2_val<float> unk1;
};
CHECK_OFFSET(AXVoice, 0x0, index);
CHECK_OFFSET(AXVoice, 0x4, state);
CHECK_OFFSET(AXVoice, 0x8, volume);
CHECK_OFFSET(AXVoice, 0xc, renderer);
CHECK_OFFSET(AXVoice, 0x10, link);
CHECK_OFFSET(AXVoice, 0x18, cbNext);
CHECK_OFFSET(AXVoice, 0x1c, priority);
CHECK_OFFSET(AXVoice, 0x20, callback);
CHECK_OFFSET(AXVoice, 0x24, userContext);
CHECK_OFFSET(AXVoice, 0x28, syncBits);
CHECK_OFFSET(AXVoice, 0x34, offsets);
CHECK_OFFSET(AXVoice, 0x48, callbackEx);
CHECK_OFFSET(AXVoice, 0x4c, callbackReason);
CHECK_OFFSET(AXVoice, 0x50, unk0);
CHECK_OFFSET(AXVoice, 0x54, unk1);
CHECK_SIZE(AXVoice, 0x58);

struct AXVoiceDeviceBusMixData
{
   be2_val<uint16_t> volume;
   be2_val<int16_t> delta;
};
CHECK_OFFSET(AXVoiceDeviceBusMixData, 0x0, volume);
CHECK_OFFSET(AXVoiceDeviceBusMixData, 0x2, delta);
CHECK_SIZE(AXVoiceDeviceBusMixData, 0x4);

struct AXVoiceDeviceMixData
{
   be2_array<AXVoiceDeviceBusMixData, 4> bus;
};
CHECK_OFFSET(AXVoiceDeviceMixData, 0x0, bus);
CHECK_SIZE(AXVoiceDeviceMixData, 0x10);

struct AXVoiceVeData
{
   be2_val<uint16_t> volume;
   be2_val<int16_t> delta;
};
CHECK_OFFSET(AXVoiceVeData, 0x0, volume);
CHECK_OFFSET(AXVoiceVeData, 0x2, delta);
CHECK_SIZE(AXVoiceVeData, 0x4);

struct AXVoiceAdpcmLoopData
{
   be2_val<uint16_t> predScale;
   be2_array<int16_t, 2> prevSample;
};
CHECK_OFFSET(AXVoiceAdpcmLoopData, 0x0, predScale);
CHECK_OFFSET(AXVoiceAdpcmLoopData, 0x2, prevSample);
CHECK_SIZE(AXVoiceAdpcmLoopData, 0x6);

struct AXVoiceAdpcm
{
   be2_array<int16_t, 16> coefficients;
   be2_val<uint16_t> gain;
   be2_val<uint16_t> predScale;
   be2_array<int16_t, 2> prevSample;
};
CHECK_OFFSET(AXVoiceAdpcm, 0x0, coefficients);
CHECK_OFFSET(AXVoiceAdpcm, 0x20, gain);
CHECK_OFFSET(AXVoiceAdpcm, 0x22, predScale);
CHECK_OFFSET(AXVoiceAdpcm, 0x24, prevSample);
CHECK_SIZE(AXVoiceAdpcm, 0x28);

// Note: "Src" = "sample rate converter", not "source"
struct AXVoiceSrc
{
   // Playback rate
   be2_val<ufixed1616_t> ratio;

   // Used by the resampler
   be2_val<ufixed016_t> currentOffsetFrac;
   be2_array<int16_t, 4> lastSample;
};
CHECK_OFFSET(AXVoiceSrc, 0x0, ratio);
CHECK_OFFSET(AXVoiceSrc, 0x4, currentOffsetFrac);
CHECK_OFFSET(AXVoiceSrc, 0x6, lastSample);
CHECK_SIZE(AXVoiceSrc, 0xe);

#pragma pack(pop)

virt_ptr<AXVoice>
AXAcquireVoice(uint32_t priority,
               AXVoiceCallbackFn callback,
               virt_ptr<void> userContext);

virt_ptr<AXVoice>
AXAcquireVoiceEx(uint32_t priority,
                 AXVoiceCallbackExFn callback,
                 virt_ptr<void> userContext);

BOOL
AXCheckVoiceOffsets(virt_ptr<AXVoiceOffsets> offsets);

void
AXFreeVoice(virt_ptr<AXVoice> voice);

uint32_t
AXGetMaxVoices();

uint32_t
AXGetVoiceCurrentOffsetEx(virt_ptr<AXVoice> voice,
                          virt_ptr<const void> samples);

uint32_t
AXGetVoiceLoopCount(virt_ptr<AXVoice> voice);

uint32_t
AXGetVoiceMixerSelect(virt_ptr<AXVoice> voice);

void
AXGetVoiceOffsetsEx(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceOffsets> offsets,
                    virt_ptr<const void> samples);

void
AXGetVoiceOffsets(virt_ptr<AXVoice> voice,
                  virt_ptr<AXVoiceOffsets> offsets);

BOOL
AXIsVoiceRunning(virt_ptr<AXVoice> voice);

void
AXSetVoiceAdpcm(virt_ptr<AXVoice> voice,
                virt_ptr<AXVoiceAdpcm> adpcm);

void
AXSetVoiceAdpcmLoop(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceAdpcmLoopData> loopData);

void
AXSetVoiceCurrentOffset(virt_ptr<AXVoice> voice,
                        uint32_t offset);

void
AXSetVoiceCurrentOffsetEx(virt_ptr<AXVoice> voice,
                          uint32_t offset,
                          virt_ptr<const void> samples);

AXResult
AXSetVoiceDeviceMix(virt_ptr<AXVoice> voice,
                    AXDeviceType type,
                    uint32_t id,
                    virt_ptr<AXVoiceDeviceMixData> mixData);

void
AXSetVoiceEndOffset(virt_ptr<AXVoice> voice,
                    uint32_t offset);

void
AXSetVoiceEndOffsetEx(virt_ptr<AXVoice> voice,
                      uint32_t offset,
                      virt_ptr<const void> samples);

AXResult
AXSetVoiceInitialTimeDelay(virt_ptr<AXVoice> voice,
                           uint16_t delay);

void
AXSetVoiceLoopOffset(virt_ptr<AXVoice> voice,
                     uint32_t offset);

void
AXSetVoiceLoopOffsetEx(virt_ptr<AXVoice> voice,
                       uint32_t offset,
                       virt_ptr<const void> samples);

void
AXSetVoiceLoop(virt_ptr<AXVoice> voice,
               AXVoiceLoop loop);

uint32_t
AXSetVoiceMixerSelect(virt_ptr<AXVoice> voice,
                      uint32_t mixerSelect);

void
AXSetVoiceOffsets(virt_ptr<AXVoice> voice,
                  virt_ptr<AXVoiceOffsets> offsets);

void
AXSetVoiceOffsetsEx(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceOffsets> offsets,
                    virt_ptr<void> samples);

void
AXSetVoicePriority(virt_ptr<AXVoice> voice,
                   uint32_t priority);

void
AXSetVoiceRmtOn(virt_ptr<AXVoice> voice,
                uint16_t on);

void
AXSetVoiceRmtIIRCoefs(virt_ptr<AXVoice> voice,
                      uint16_t filter,
                      var_args);

void
AXSetVoiceSrc(virt_ptr<AXVoice> voice,
              virt_ptr<AXVoiceSrc> src);

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(virt_ptr<AXVoice> voice,
                   float ratio);

void
AXSetVoiceSrcType(virt_ptr<AXVoice> voice,
                  AXVoiceSrcType type);

void
AXSetVoiceState(virt_ptr<AXVoice> voice,
                AXVoiceState state);

void
AXSetVoiceType(virt_ptr<AXVoice> voice,
               AXVoiceType type);

void
AXSetVoiceVe(virt_ptr<AXVoice> voice,
             virt_ptr<AXVoiceVeData> veData);

void
AXSetVoiceVeDelta(virt_ptr<AXVoice> voice,
                  int16_t delta);

namespace internal
{

#pragma pack(push, 1)

struct AXCafeVoiceData
{
   be2_val<AXVoiceLoop> loopFlag;
   be2_val<AXVoiceFormat> format;
   be2_val<uint16_t> memPageNumber;
   be2_val<virt_addr> loopOffsetAbs;
   be2_val<virt_addr> endOffsetAbs;
   be2_val<virt_addr> currentOffsetAbs;
};
CHECK_OFFSET(AXCafeVoiceData, 0x0, loopFlag);
CHECK_OFFSET(AXCafeVoiceData, 0x2, format);
CHECK_OFFSET(AXCafeVoiceData, 0x4, memPageNumber);
CHECK_OFFSET(AXCafeVoiceData, 0x6, loopOffsetAbs);
CHECK_OFFSET(AXCafeVoiceData, 0xa, endOffsetAbs);
CHECK_OFFSET(AXCafeVoiceData, 0xe, currentOffsetAbs);
CHECK_SIZE(AXCafeVoiceData, 0x12);

struct AXCafeVoiceExtras
{
   UNKNOWN(0x8);

   be2_val<uint16_t> srcMode;
   be2_val<uint16_t> srcModeUnk;

   UNKNOWN(0x2);

   be2_val<AXVoiceType> type;

   UNKNOWN(0x15a);

   be2_val<uint16_t> state;

   be2_val<uint16_t> itdOn;

   UNKNOWN(0x2);

   be2_val<uint16_t> itdDelay;

   UNKNOWN(0x8);

   be2_struct<AXVoiceVeData> ve;

   be2_struct<AXCafeVoiceData> data;

   be2_struct<AXVoiceAdpcm> adpcm;

   be2_struct<AXVoiceSrc> src;

   be2_struct<AXVoiceAdpcmLoopData> adpcmLoop;

   UNKNOWN(0xe4);

   be2_val<uint32_t> syncBits;

   UNKNOWN(0xc);
};
CHECK_OFFSET(AXCafeVoiceExtras, 0x8, srcMode);
CHECK_OFFSET(AXCafeVoiceExtras, 0xa, srcModeUnk);
CHECK_OFFSET(AXCafeVoiceExtras, 0xe, type);
CHECK_OFFSET(AXCafeVoiceExtras, 0x16a, state);
CHECK_OFFSET(AXCafeVoiceExtras, 0x16c, itdOn);
CHECK_OFFSET(AXCafeVoiceExtras, 0x170, itdDelay);
CHECK_OFFSET(AXCafeVoiceExtras, 0x17a, ve);
CHECK_OFFSET(AXCafeVoiceExtras, 0x17e, data);
CHECK_OFFSET(AXCafeVoiceExtras, 0x190, adpcm);
CHECK_OFFSET(AXCafeVoiceExtras, 0x1b8, src);
CHECK_OFFSET(AXCafeVoiceExtras, 0x1c6, adpcmLoop);
CHECK_OFFSET(AXCafeVoiceExtras, 0x2b0, syncBits);
CHECK_SIZE(AXCafeVoiceExtras, 0x2c0);

struct AXVoiceExtras : AXCafeVoiceExtras
{
   struct MixVolume
   {
      ufixed_1_15_t volume;
      ufixed_1_15_t delta;
   };

   // Volume on each of 6 surround channels for TV output
   MixVolume tvVolume[AXNumTvDevices][AXNumTvChannels][AXNumTvBus];

   // Volume on each of 4 channels for DRC output (2 stereo channels + ???)
   MixVolume drcVolume[AXNumDrcDevices][AXNumDrcChannels][AXNumDrcBus];

   // Volume for each of 4 controller speakers
   MixVolume rmtVolume[AXNumRmtDevices][AXNumRmtChannels][AXNumRmtBus];

   // Number of loops so far
   uint32_t loopCount;

   // Used during decoding
   uint32_t numSamples;
   Pcm16Sample samples[144];

};

#pragma pack(pop)

void
initVoices();

void
setVoiceAddresses(virt_ptr<AXVoice> voice,
                  AXCafeVoiceData &offsets);

const std::vector<virt_ptr<AXVoice>>
getAcquiredVoices();

virt_ptr<AXVoiceExtras>
getVoiceExtras(int index);

} // namespace internal

} // namespace cafe::sndcore2
