#include "sndcore2.h"
#include "sndcore2_config.h"
#include "sndcore2_voice.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_interrupts.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/coreinit/coreinit_time.h"
#include "decaf_sound.h"

#include <common/log.h>
#include <fmt/format.h>

using namespace cafe::coreinit;

namespace cafe::sndcore2
{

static const size_t MaxFrameCallbacks = 64;

// Enough for 6 channels of 3ms at 48kHz
static constexpr auto AXNumBufferSamples = 6 * 144;

struct StaticConfigData
{
   be2_val<BOOL> initialised;
   be2_val<int32_t> outputRate;
   be2_val<int32_t> outputChannels;
   be2_val<uint32_t> defaultMixerSelect;
   be2_struct<OSAlarm> frameAlarm;
   be2_array<AXFrameCallback, MaxFrameCallbacks> appFrameCallbacks;
   be2_array<int32_t, AXNumBufferSamples> mixBuffer;
   be2_array<int16_t, AXNumBufferSamples> outputBuffer;
   be2_val<AXFrameCallback> frameCallback;
   be2_struct<OSThread> frameCallbackThread;
   be2_struct<OSThreadQueue> frameCallbackThreadQueue;
   be2_array<char, 32> frameCallbackThreadName;
   be2_array<uint8_t, 16 * 1024> frameCallbackThreadStack;
};

static OSThreadEntryPointFn FrameCallbackThreadEntryPoint = nullptr;
static AlarmCallbackFn FrameAlarmHandler = nullptr;
static virt_ptr<StaticConfigData> sConfigData = nullptr;

static std::atomic<int32_t>
sProtectLock = { 0 };

void
AXInit()
{
   StackObject<AXInitParams> params;
   params->renderer = AXRendererFreq::Freq32khz;
   params->pipeline = AXInitPipeline::Single;
   AXInitWithParams(params);
}

void
AXInitWithParams(virt_ptr<AXInitParams> params)
{
   if (AXIsInit()) {
      return;
   }

   switch (params->renderer) {
   case AXRendererFreq::Freq32khz:
      sConfigData->outputRate = 32000;
      break;
   case AXRendererFreq::Freq48khz:
      sConfigData->outputRate = 48000;
      break;
   default:
      decaf_abort(fmt::format("Unimplemented AXInitRenderer {}", params->renderer));
   }

   sConfigData->outputChannels = 2;  // TODO: surround support
   internal::initVoices();
   internal::initEvents();

   if (auto driver = decaf::getSoundDriver()) {
      if (!driver->start(sConfigData->outputRate,
                         sConfigData->outputChannels)) {
         gLog->error("Sound driver failed to start, disabling sound output");
         decaf::setSoundDriver(nullptr);
      }
   }

   sConfigData->initialised = TRUE;
}

BOOL
AXIsInit()
{
   return sConfigData->initialised;
}

void
AXQuit()
{
   decaf_warn_stub();
}

void
AXInitProfile(virt_ptr<AXProfile> profile,
              uint32_t count)
{
   decaf_warn_stub();
}

AXRendererFreq
AXGetRendererFreq()
{
   if (sConfigData->outputRate == 32000) {
      return AXRendererFreq::Freq32khz;
   } else {
      return AXRendererFreq::Freq48khz;
   }
}

uint32_t
AXGetSwapProfile(virt_ptr<AXProfile> profile,
                 uint32_t count)
{
   decaf_warn_stub();
   return 0;
}

uint32_t
AXGetDefaultMixerSelect()
{
   decaf_warn_stub();
   return sConfigData->defaultMixerSelect;
}

AXResult
AXSetDefaultMixerSelect(uint32_t defaultMixerSelect)
{
   decaf_warn_stub();
   sConfigData->defaultMixerSelect = defaultMixerSelect;
   return AXResult::Success;
}

AXResult
AXRegisterAppFrameCallback(AXFrameCallback callback)
{
   if (!callback) {
      return AXResult::CallbackInvalid;
   }

   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      if (sConfigData->appFrameCallbacks[i] == callback) {
         decaf_abort("Application double-registered app frame callback");
      }
   }

   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      if (!sConfigData->appFrameCallbacks[i]) {
         sConfigData->appFrameCallbacks[i] = callback;
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
      if (sConfigData->appFrameCallbacks[i] == callback) {
         sConfigData->appFrameCallbacks[i] = nullptr;
         return AXResult::Success;
      }
   }

   return AXResult::CallbackNotFound;
}

AXFrameCallback
AXRegisterFrameCallback(AXFrameCallback callback)
{
   auto oldCallback = sConfigData->frameCallback;
   sConfigData->frameCallback = callback;
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

uint32_t
AXGetInputSamplesPerFrame()
{
   if (sConfigData->outputRate == 32000) {
      return 96;
   } else if (sConfigData->outputRate == 48000) {
      return 144;
   } else {
      decaf_abort(fmt::format("Unexpected output rate {}", sConfigData->outputRate));
   }
}

uint32_t
AXGetInputSamplesPerSec()
{
   return (AXGetInputSamplesPerFrame() / 3) * 1000;
}

void
AXPrepareEfxData(virt_ptr<void> buffer,
                 uint32_t size)
{
   // Nothing to do here, we have implicit cache coherency
}

namespace internal
{

static uint32_t
frameCallbackThreadEntry(uint32_t core_id,
                         virt_ptr<void>)
{
   static const int NumOutputSamples = (48000 * 3) / 1000;
   int numInputSamples = sConfigData->outputRate * 3 / 1000;

   while (true) {
      coreinit::internal::lockScheduler();
      coreinit::internal::sleepThreadNoLock(virt_addrof(sConfigData->frameCallbackThreadQueue));
      coreinit::internal::rescheduleSelfNoLock();
      coreinit::internal::unlockScheduler();

      if (sConfigData->frameCallback) {
         cafe::invoke(cpu::this_core::state(),
                      sConfigData->frameCallback);
      }

      for (auto i = 0; i < MaxFrameCallbacks; ++i) {
         if (sConfigData->appFrameCallbacks[i]) {
            cafe::invoke(cpu::this_core::state(),
                         sConfigData->appFrameCallbacks[i]);
         }
      }

      decaf_check(static_cast<size_t>(NumOutputSamples * sConfigData->outputChannels) <= sConfigData->mixBuffer.size());
      internal::mixOutput(virt_addrof(sConfigData->mixBuffer), numInputSamples, sConfigData->outputChannels);

      auto driver = decaf::getSoundDriver();

      if (driver) {
         for (int i = 0; i < NumOutputSamples * sConfigData->outputChannels; ++i) {
            sConfigData->outputBuffer[i] = static_cast<int16_t>(std::min<int32_t>(std::max<int32_t>(sConfigData->mixBuffer[i], -32768), 32767));
         }

         driver->output(virt_addrof(sConfigData->outputBuffer).getRawPointer(),
                        NumOutputSamples);
      }
   }

   return 0;
}

static void
startFrameAlarmThread()
{
   auto thread = virt_addrof(sConfigData->frameCallbackThread);
   auto stack = virt_addrof(sConfigData->frameCallbackThreadStack);
   auto stackSize = sConfigData->frameCallbackThreadStack.size();
   sConfigData->frameCallbackThreadName = "AX Callback Thread";

   OSCreateThread(thread,
                  FrameCallbackThreadEntryPoint,
                  0, nullptr,
                  virt_cast<uint32_t *>(stack + stackSize),
                  stackSize, -1,
                  static_cast<OSThreadAttributes>(1 << cpu::this_core::id()));
   OSSetThreadName(thread, virt_addrof(sConfigData->frameCallbackThreadName));
   OSResumeThread(thread);
}

static void
frameAlarmHandler(virt_ptr<OSAlarm> alarm,
                  virt_ptr<OSContext> context)
{
   coreinit::internal::lockScheduler();
   coreinit::internal::wakeupThreadNoLock(virt_addrof(sConfigData->frameCallbackThreadQueue));
   coreinit::internal::unlockScheduler();
}

void
initEvents()
{
   using namespace coreinit;

   sConfigData->frameCallback = nullptr;
   for (auto i = 0; i < MaxFrameCallbacks; ++i) {
      sConfigData->appFrameCallbacks[i] = nullptr;
   }

   startFrameAlarmThread();

   auto ticks = static_cast<OSTime>(OSGetSystemInfo()->busSpeed / 4) * 3 / 1000;
   OSCreateAlarm(virt_addrof(sConfigData->frameAlarm));
   OSSetPeriodicAlarm(virt_addrof(sConfigData->frameAlarm),
                      OSGetTime(), ticks,
                      FrameAlarmHandler);
}

int
getOutputRate()
{
   return sConfigData->outputRate;
}

} // namespace internal

void
Library::registerConfigSymbols()
{
   RegisterFunctionExport(AXInit);
   RegisterFunctionExport(AXInitWithParams);
   RegisterFunctionExport(AXIsInit);
   RegisterFunctionExport(AXQuit);
   RegisterFunctionExport(AXInitProfile);
   RegisterFunctionExport(AXGetRendererFreq);
   RegisterFunctionExport(AXGetSwapProfile);
   RegisterFunctionExport(AXGetDefaultMixerSelect);
   RegisterFunctionExport(AXSetDefaultMixerSelect);
   RegisterFunctionExport(AXRegisterAppFrameCallback);
   RegisterFunctionExport(AXDeregisterAppFrameCallback);
   RegisterFunctionExport(AXRegisterFrameCallback);
   RegisterFunctionExportName("AXRegisterCallback", AXRegisterFrameCallback);
   RegisterFunctionExport(AXUserBegin);
   RegisterFunctionExport(AXUserEnd);
   RegisterFunctionExport(AXUserIsProtected);
   RegisterFunctionExport(AXGetInputSamplesPerFrame);
   RegisterFunctionExport(AXGetInputSamplesPerSec);
   RegisterFunctionExport(AXPrepareEfxData);

   RegisterDataInternal(sConfigData);
   RegisterFunctionInternal(internal::frameAlarmHandler, FrameAlarmHandler);
   RegisterFunctionInternal(internal::frameCallbackThreadEntry, FrameCallbackThreadEntryPoint);
}

} // namespace cafe::sndcore2
