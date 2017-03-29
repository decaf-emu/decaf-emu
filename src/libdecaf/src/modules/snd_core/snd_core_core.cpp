#include "snd_core.h"
#include "snd_core_core.h"
#include "snd_core_voice.h"
#include "decaf_sound.h"
#include "modules/coreinit/coreinit_alarm.h"
#include "modules/coreinit/coreinit_interrupts.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_time.h"
#include "ppcutils/wfunc_ptr.h"
#include "ppcutils/wfunc_call.h"

namespace snd_core
{

static const size_t MaxFrameCallbacks = 64;

static BOOL
gAXInit = FALSE;

static int
sOutputRate;

static int
sOutputChannels;

static coreinit::OSThreadEntryPointFn
sFrameCallbackThreadEntryPoint;

static coreinit::AlarmCallback
sFrameAlarmHandler = nullptr;

static coreinit::OSAlarm *
sFrameAlarm = nullptr;

static coreinit::OSThread *
sFrameCallbackThread;

static coreinit::OSThreadQueue *
sFrameCallbackThreadQueue;

static AXFrameCallback
sFrameCallback = nullptr;

static AXFrameCallback
sAppFrameCallbacks[MaxFrameCallbacks] = { nullptr };

static std::atomic<int32_t>
sProtectLock = { 0 };

static std::array<int32_t, 6*144>  // Enough for 6 channels of 3ms at 48kHz
sMixBuffer;

static std::array<int16_t, 6*144>
sOutputBuffer;

void
AXInit()
{
   AXInitParams params;
   params.renderer = AXInitRenderer::Out32khz;
   params.pipeline = AXInitPipeline::Single;
   AXInitWithParams(&params);
}

void
AXInitWithParams(AXInitParams *params)
{
   if (AXIsInit()) {
      return;
   }

   switch (params->renderer) {
   case AXInitRenderer::Out32khz:
      sOutputRate = 32000;
      break;
   case AXInitRenderer::Out48khz:
      sOutputRate = 48000;
      break;
   default:
      decaf_abort(fmt::format("Unimplemented AXInitRenderer {}", params->renderer));
   }

   sOutputChannels = 2;  // TODO: surround support
   internal::initVoices();
   internal::initEvents();

   if (auto driver = decaf::getSoundDriver()) {
      if (!driver->start(48000, sOutputChannels)) {
         gLog->error("Sound driver failed to start, disabling sound output");
         decaf::setSoundDriver(nullptr);
      }
   }

   gAXInit = TRUE;
}

BOOL
AXIsInit()
{
   return gAXInit;
}

void
AXInitProfile(AXProfile *profile, uint32_t count)
{
   // TODO: AXInitProfile
}

uint32_t
AXGetSwapProfile(AXProfile *profile, uint32_t count)
{
   return 0;
}

AXResult
AXSetDefaultMixerSelect(uint32_t)
{
   // TODO: AXSetDefaultMixerSelect
   return AXResult::Success;
}

AXResult
AXRegisterAppFrameCallback(AXFrameCallback callback)
{
   if (!callback) {
      return AXResult::CallbackInvalid;
   }

   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      if (sAppFrameCallbacks[i] == callback) {
         decaf_abort("Application double-registered app frame callback");
      }
   }

   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      if (!sAppFrameCallbacks[i]) {
         sAppFrameCallbacks[i] = callback;
         return AXResult::Success;
      }
   }

   return AXResult::TooManyCallbacks;
}

AXResult
AXDeregisterAppFrameCallback(AXFrameCallback callback)
{
   if (!callback) {
      return AXResult::CallbackInvalid;
   }

   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      if (sAppFrameCallbacks[i] == callback) {
         sAppFrameCallbacks[i] = nullptr;
         return AXResult::Success;
      }
   }

   return AXResult::CallbackNotFound;
}

AXFrameCallback
AXRegisterFrameCallback(AXFrameCallback callback)
{
   auto oldCallback = sFrameCallback;
   sFrameCallback = callback;
   return oldCallback;
}

int32_t
AXUserBegin()
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return sProtectLock.fetch_add(1);
}

int32_t
AXUserEnd()
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return sProtectLock.fetch_sub(1);
}

BOOL
AXUserIsProtected()
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return sProtectLock.load() > 0;
}

int32_t
AXVoiceBegin(AXVoice *voice)
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return AXUserBegin();
}

int32_t
AXVoiceEnd(AXVoice *voice)
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return AXUserEnd();
}

BOOL
AXVoiceIsProtected(AXVoice *voice)
{
   decaf_warn_stub();

   return FALSE;
}

uint32_t
AXGetInputSamplesPerFrame()
{
   if (sOutputRate == 32000) {
      return 96;
   } else if (sOutputRate == 48000) {
      return 144;
   } else {
      decaf_abort(fmt::format("Unexpected output rate {}", sOutputRate));
   }
}

uint32_t
AXGetInputSamplesPerSec()
{
   return (AXGetInputSamplesPerFrame() / 3) * 1000;
}

int32_t
AXRmtGetSamplesLeft()
{
   decaf_warn_stub();

   return 0;
}

int32_t
AXRmtGetSamples(int32_t, be_val<uint8_t> *buffer, int32_t samples)
{
   decaf_warn_stub();

   return 0;
}

int32_t
AXRmtAdvancePtr(int32_t)
{
   decaf_warn_stub();

   return 0;
}

void
AXPrepareEfxData(void *buffer, uint32_t size)
{
   // Nothing to do here, we have implicit cache coherency
}

namespace internal
{

uint32_t
frameCallbackThreadEntry(uint32_t core_id,
                         void *arg2)
{
   static const int NumOutputSamples = 48000 * 3 / 1000;
   int numInputSamples = sOutputRate * 3 / 1000;

   while (true) {
      coreinit::internal::lockScheduler();
      coreinit::internal::sleepThreadNoLock(sFrameCallbackThreadQueue);
      coreinit::internal::rescheduleSelfNoLock();
      coreinit::internal::unlockScheduler();

      if (sFrameCallback) {
         sFrameCallback();
      }
      for (auto i = 0; i < MaxFrameCallbacks; ++i) {
         if (sAppFrameCallbacks[i]) {
            sAppFrameCallbacks[i]();
         }
      }

      decaf_check(static_cast<size_t>(NumOutputSamples * sOutputChannels) <= sMixBuffer.size());
      internal::mixOutput(&sMixBuffer[0], numInputSamples, sOutputChannels);

      auto driver = decaf::getSoundDriver();

      if (driver) {
         for (int i = 0; i < NumOutputSamples * sOutputChannels; ++i) {
            sOutputBuffer[i] = static_cast<int16_t>(std::min(std::max(sMixBuffer[i], -32768), 32767));
         }

         driver->output(&sOutputBuffer[0], NumOutputSamples);
      }
   }

   return 0;
}

void
startFrameAlarmThread()
{
   using namespace coreinit;

   auto stackSize = 16 * 1024;
   auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
   auto name = coreinit::internal::sysStrDup("AX Callback Thread");

   OSCreateThread(sFrameCallbackThread, sFrameCallbackThreadEntryPoint, 0, nullptr,
      reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -1,
      static_cast<OSThreadAttributes>(1 << cpu::this_core::id()));
   OSSetThreadName(sFrameCallbackThread, name);
   OSResumeThread(sFrameCallbackThread);
}

void
frameAlarmHandler(coreinit::OSAlarm *alarm,
                  coreinit::OSContext *context)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::wakeupThreadNoLock(sFrameCallbackThreadQueue);
   coreinit::internal::unlockScheduler();
}

void
initEvents()
{
   using namespace coreinit;

   sFrameCallback = nullptr;
   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      sAppFrameCallbacks[i] = nullptr;
   }

   startFrameAlarmThread();

   sFrameAlarm = coreinit::internal::sysAlloc<OSAlarm>();
   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->busSpeed / 4) * 3 / 1000;
   OSCreateAlarm(sFrameAlarm);
   OSSetPeriodicAlarm(sFrameAlarm, OSGetTime(), ticks, sFrameAlarmHandler);
}

int
getOutputRate()
{
   return sOutputRate;
}

} // namespace internal

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(AXInit);
   RegisterKernelFunction(AXInitWithParams);
   RegisterKernelFunction(AXIsInit);
   RegisterKernelFunction(AXInitProfile);
   RegisterKernelFunction(AXGetSwapProfile);
   RegisterKernelFunction(AXSetDefaultMixerSelect);
   RegisterKernelFunction(AXRegisterAppFrameCallback);
   RegisterKernelFunction(AXDeregisterAppFrameCallback);
   RegisterKernelFunction(AXRegisterFrameCallback);
   RegisterKernelFunctionName("AXRegisterCallback", AXRegisterFrameCallback);
   RegisterKernelFunction(AXUserBegin);
   RegisterKernelFunction(AXUserEnd);
   RegisterKernelFunction(AXVoiceBegin);
   RegisterKernelFunction(AXVoiceEnd);
   RegisterKernelFunction(AXUserIsProtected);
   RegisterKernelFunction(AXVoiceIsProtected);
   RegisterKernelFunction(AXGetInputSamplesPerFrame);
   RegisterKernelFunction(AXGetInputSamplesPerSec);
   RegisterKernelFunction(AXRmtGetSamples);
   RegisterKernelFunction(AXRmtGetSamplesLeft);
   RegisterKernelFunction(AXRmtAdvancePtr);
   RegisterKernelFunction(AXPrepareEfxData);

   RegisterInternalFunction(internal::frameAlarmHandler, sFrameAlarmHandler);
   RegisterInternalFunction(internal::frameCallbackThreadEntry, sFrameCallbackThreadEntryPoint);
   RegisterInternalData(sFrameCallbackThreadQueue);
   RegisterInternalData(sFrameCallbackThread);
}

} // namespace snd_core
