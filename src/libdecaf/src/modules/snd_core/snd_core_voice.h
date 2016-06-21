#pragma once
#include "common/be_val.h"
#include "common/structsize.h"
#include "common/types.h"

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

struct AXVoice
{
   // The index of this voice out of the total voices
   be_val<uint32_t> index;

   UNKNOWN(0xC);

   // this is a link used in the stack, we do this in host-memory currently
   AXVoiceLink link;

   UNKNOWN(0x4);

   // The priority of this voice used for force-acquiring a voice
   be_val<uint32_t> priority;

   // The callback to call if this is force-free'd by another acquire
   be_AXVoiceCallbackFn callback;

   // The user context to send to the callbacks
   be_ptr<void> userContext;

   UNKNOWN(0x20);

   // An extended version of the callback above
   be_AXVoiceCallbackExFn callbackEx;

   UNKNOWN(0xC);
};
CHECK_OFFSET(AXVoice, 0x0, index);
CHECK_OFFSET(AXVoice, 0x10, link);
CHECK_OFFSET(AXVoice, 0x1c, priority);
CHECK_OFFSET(AXVoice, 0x20, callback);
CHECK_OFFSET(AXVoice, 0x24, userContext);
CHECK_OFFSET(AXVoice, 0x48, callbackEx);
CHECK_SIZE(AXVoice, 0x58);

AXVoice *
AXAcquireVoice(uint32_t priority, AXVoiceCallbackFn callback, void *userContext);

AXVoice *
AXAcquireVoiceEx(uint32_t priority, AXVoiceCallbackExFn callback, void *userContext);

void
AXFreeVoice(AXVoice *voice);

void
AXSetVoiceVe(AXVoice *voice, void *_unk);

BOOL
AXIsVoiceRunning(AXVoice *voice);

namespace internal
{

void initVoices();

} // namespace internal

} // namespace snd_core
