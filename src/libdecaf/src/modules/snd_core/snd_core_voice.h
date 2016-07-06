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
   UNKNOWN(0x02);
   be_val<AXVoiceLoop> loopingEnabled;
   be_val<uint32_t> loopOffset;
   be_val<uint32_t> endOffset;
   be_val<uint32_t> currentOffset;
   be_ptr<void> data;
};
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

   UNKNOWN(0x8);

   //! this is a link used in the stack, we do this in host-memory currently
   AXVoiceLink link;

   UNKNOWN(0x4);

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

   UNKNOWN(0xC);

   // float at 0x50
   // float at 0x54
};
CHECK_OFFSET(AXVoice, 0x0, index);
CHECK_OFFSET(AXVoice, 0x10, link);
CHECK_OFFSET(AXVoice, 0x1c, priority);
CHECK_OFFSET(AXVoice, 0x20, callback);
CHECK_OFFSET(AXVoice, 0x24, userContext);
CHECK_OFFSET(AXVoice, 0x34, offsets);
CHECK_OFFSET(AXVoice, 0x48, callbackEx);
CHECK_SIZE(AXVoice, 0x58);

// TODO: Reverse AXVoiceDeviceMixData
struct AXVoiceDeviceMixData;

// TODO: Reverse AXVoiceVeData
struct AXVoiceVeData;

// TODO: Reverse AXVoiceAdpcmLoopData
struct AXVoiceAdpcmLoopData;

// TODO: Reverse AXVoiceAdpcm
struct AXVoiceAdpcm;

// TODO: Reverse AXVoiceSrc
struct AXVoiceSrc;

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
AXSetVoiceLoop(AXVoice *voice,
               AXVoiceLoop loop);

void
AXSetVoiceOffsets(AXVoice *voice,
                  AXVoiceOffsets *offsets);

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

void initVoices();

} // namespace internal

} // namespace snd_core
