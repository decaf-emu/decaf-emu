#pragma once
#include "common/be_val.h"
#include "common/structsize.h"
#include "common/types.h"
#include "snd_core_device.h"
#include "snd_core_enum.h"

namespace snd_core
{

using AXVoiceCallbackFn = wfunc_ptr<void*>;
using be_AXVoiceCallbackFn = be_wfunc_ptr<void*>;

using AXVoiceCallbackExFn = wfunc_ptr<void*, uint32_t, uint32_t>;
using be_AXVoiceCallbackExFn = be_wfunc_ptr<void*, uint32_t, uint32_t>;

struct AXVoice;

struct AXVoiceLink
{
   be_ptr<AXVoice> next;
   be_ptr<AXVoice> prev;
};
CHECK_OFFSET(AXVoiceLink, 0x0, next);
CHECK_OFFSET(AXVoiceLink, 0x4, prev);
CHECK_SIZE(AXVoiceLink, 0x8);

struct AXVoiceOffsets
{
   be_val<AXVoiceFormat> dataType;
   be_val<AXVoiceLoop> loopingEnabled;
   be_val<uint32_t> loopOffset;
   be_val<uint32_t> endOffset;
   be_val<uint32_t> currentOffset;
   be_ptr<const void> data;
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
   be_val<uint32_t> index;

   //! Current play state of this voice
   be_val<AXVoiceState> state;

   //! Current volume of this voice
   be_val<uint32_t> volume;

   //! The renderer to use for this voice
   be_val<AXRenderer> renderer;

   //! this is a link used in the stack, we do this in host-memory currently
   AXVoiceLink link;

   //! A link to the next callback to invoke
   be_ptr<AXVoice> cbNext;

   //! The priority of this voice used for force-acquiring a voice
   be_val<uint32_t> priority;

   //! The callback to call if this is force-free'd by another acquire
   be_AXVoiceCallbackFn callback;

   //! The user context to send to the callbacks
   be_ptr<void> userContext;

   UNKNOWN(0xc);

   //! The current offset data!
   AXVoiceOffsets offsets;

   //! An extended version of the callback above
   be_AXVoiceCallbackExFn callbackEx;

   //! The reason for the callback being invoked
   be_val<uint32_t> callbackReason;

   be_val<float> unk0;
   be_val<float> unk1;
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
CHECK_OFFSET(AXVoice, 0x34, offsets);
CHECK_OFFSET(AXVoice, 0x48, callbackEx);
CHECK_OFFSET(AXVoice, 0x4c, callbackReason);
CHECK_OFFSET(AXVoice, 0x50, unk0);
CHECK_OFFSET(AXVoice, 0x54, unk1);
CHECK_SIZE(AXVoice, 0x58);

struct AXVoiceDeviceBusMixData
{
   be_val<uint16_t> volume;
   be_val<int16_t> delta;
};
CHECK_OFFSET(AXVoiceDeviceBusMixData, 0x0, volume);
CHECK_SIZE(AXVoiceDeviceBusMixData, 0x4);

struct AXVoiceDeviceMixData
{
   AXVoiceDeviceBusMixData bus[4];
};
CHECK_OFFSET(AXVoiceDeviceMixData, 0x0, bus);
CHECK_SIZE(AXVoiceDeviceMixData, 0x10);

struct AXVoiceVeData
{
   be_val<uint16_t> volume;
   be_val<int16_t> delta;
};
CHECK_OFFSET(AXVoiceVeData, 0x0, volume);
CHECK_OFFSET(AXVoiceVeData, 0x2, delta);
CHECK_SIZE(AXVoiceVeData, 0x4);

struct AXVoiceAdpcmLoopData
{
   be_val<uint16_t> predScale;
   be_val<int16_t> prevSample[2];
};
CHECK_OFFSET(AXVoiceAdpcmLoopData, 0x0, predScale);
CHECK_OFFSET(AXVoiceAdpcmLoopData, 0x2, prevSample);
CHECK_SIZE(AXVoiceAdpcmLoopData, 0x6);

struct AXVoiceAdpcm
{
   be_val<int16_t> coefficients[16];
   be_val<uint16_t> gain;
   be_val<uint16_t> predScale;
   be_val<int16_t> prevSample[2];
};
CHECK_OFFSET(AXVoiceAdpcm, 0x0, coefficients);
CHECK_OFFSET(AXVoiceAdpcm, 0x20, gain);
CHECK_OFFSET(AXVoiceAdpcm, 0x22, predScale);
CHECK_OFFSET(AXVoiceAdpcm, 0x24, prevSample);
CHECK_SIZE(AXVoiceAdpcm, 0x28);

// Note: "Src" = "sample rate converter", not "source"
struct AXVoiceSrc
{
    // Playback rate as a 16.16 fixed-point ratio
    be_val<uint16_t> ratioInt;
    be_val<uint16_t> ratioFrac;
    // Used by the resampler
    be_val<uint16_t> currentOffsetFrac;
    be_val<int16_t> lastSample[4];
};

AXVoice *
AXAcquireVoice(uint32_t priority,
               AXVoiceCallbackFn callback,
               void *userContext);

AXVoice *
AXAcquireVoiceEx(uint32_t priority,
                 AXVoiceCallbackExFn callback,
                 void *userContext);

BOOL
AXCheckVoiceOffsets(AXVoiceOffsets *offsets);

void
AXFreeVoice(AXVoice *voice);

uint32_t
AXGetMaxVoices();

uint32_t
AXGetVoiceCurrentOffsetEx(AXVoice *voice,
                          const void *samples);

uint32_t
AXGetVoiceLoopCount(AXVoice *voice);

void
AXGetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets);

BOOL
AXIsVoiceRunning(AXVoice *voice);

void
AXSetVoiceAdpcm(AXVoice *voice,
                AXVoiceAdpcm *adpcm);

void
AXSetVoiceAdpcmLoop(AXVoice *voice,
                    AXVoiceAdpcmLoopData *loopData);

void
AXSetVoiceCurrentOffset(AXVoice *voice,
                        uint32_t offset);

AXResult
AXSetVoiceDeviceMix(AXVoice *voice,
                    AXDeviceType type,
                    uint32_t id,
                    AXVoiceDeviceMixData *mixData);

void
AXSetVoiceEndOffset(AXVoice *voice,
                    uint32_t offset);

void
AXSetVoiceEndOffsetEx(AXVoice *voice,
                      uint32_t offset,
                      const void *samples);

void
AXSetVoiceLoopOffset(AXVoice *voice,
                     uint32_t offset);

void
AXSetVoiceLoopOffsetEx(AXVoice *voice,
                       uint32_t offset,
                       const void *samples);

void
AXSetVoiceLoop(AXVoice *voice,
               AXVoiceLoop loop);

void
AXSetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets);

void
AXSetVoicePriority(AXVoice *voice,
                   uint32_t priority);

void
AXSetVoiceRmtIIRCoefs(AXVoice *voice,
                      uint16_t filter,
                      ppctypes::VarList &args);

void
AXSetVoiceSrc(AXVoice *voice,
              AXVoiceSrc *src);

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(AXVoice *voice,
                   float ratio);

void
AXSetVoiceSrcType(AXVoice *voice,
                  AXVoiceSrcType type);

void
AXSetVoiceState(AXVoice *voice,
                AXVoiceState state);

void
AXSetVoiceType(AXVoice *voice,
               AXVoiceType type);

void
AXSetVoiceVe(AXVoice *voice,
             AXVoiceVeData *veData);

namespace internal
{

struct AXVoiceExtras
{
   // Volume on each of 6 surround channels for TV output
   uint16_t tvVolume[6];
   // Volume on each of 4 channels for DRC output (2 stereo channels + ???)
   uint16_t drcVolume[4];
   // Volume for each of 4 controller speakers
   uint16_t controllerVolume[4];

   // Playback rate ratio (number of output samples per input sample)
   //  in 16.16 fixed point
   uint32_t playbackRatio;
   // Current fractional offset (as 16.16 fixed point)
   int32_t offsetFrac;
   // Sample index of next sample to read
   uint32_t readPosition;
   // Current sample value (at index readPosition-1)
   int16_t currentSample;
   // Previous sample value (for interpolated resampling)
   int16_t prevSample;

   // Number of loops so far
   uint32_t loopCount;

   // ADPCM decoding data
   int16_t adpcmCoeff[16];
   uint8_t adpcmPredScale;
   int16_t adpcmPrevSample[2];
   // ADPCM data for loop start point
   uint8_t adpcmLoopPredScale;
   int16_t adpcmLoopPrevSample[2];
};

void
initVoices();

const std::vector<AXVoice*>
getAcquiredVoices();

AXVoiceExtras *
getVoiceExtras(int index);

} // namespace internal

} // namespace snd_core
