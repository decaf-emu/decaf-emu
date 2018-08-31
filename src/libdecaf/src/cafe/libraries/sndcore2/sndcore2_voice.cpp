#include "sndcore2.h"
#include "sndcore2_config.h"
#include "sndcore2_constants.h"
#include "sndcore2_voice.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "decaf_config.h"
#include "decaf_sound.h"

#include <common/decaf_assert.h>
#include <common/platform_dir.h>
#include <array>
#include <fmt/format.h>
#include <fstream>
#include <queue>

namespace cafe::sndcore2
{

struct StaticVoiceData
{
   be2_array<AXVoice, AXMaxNumVoices> voices;
   be2_array<internal::AXVoiceExtras, AXMaxNumVoices> voiceExtras;
};

// TODO: Move away from std::vector
static std::vector<virt_ptr<AXVoice>>
sAcquiredVoices;

// TODO: Move away from std::queue
static std::queue<virt_ptr<AXVoice>>
sAvailVoiceStack;

static virt_ptr<StaticVoiceData>
sVoiceData = nullptr;

virt_ptr<AXVoice>
AXAcquireVoice(uint32_t priority,
               AXVoiceCallbackFn callback,
               virt_ptr<void> userContext)
{
   return AXAcquireVoiceEx(priority,
                           virt_func_cast<AXVoiceCallbackExFn>(virt_func_cast<virt_addr>(callback)),
                           userContext);
}

virt_ptr<AXVoice>
AXAcquireVoiceEx(uint32_t priority,
                 AXVoiceCallbackExFn callback,
                 virt_ptr<void> userContext)
{
   if (sAvailVoiceStack.empty()) {
      // If there are no available voices, try to force-deallocate one...
      for (auto &i : sAcquiredVoices) {
         if (i->priority < priority) {
            // TODO: Send callback for forcing the FreeVoice
            AXFreeVoice(i);
            break;
         }
      }
   }

   if (sAvailVoiceStack.empty()) {
      // No voices available to acquire
      return nullptr;
   }

   // Grab our voice from the stack
   auto foundVoice = sAvailVoiceStack.front();
   sAvailVoiceStack.pop();

   // Reset the voice
   auto voiceIndex = foundVoice->index;
   std::memset(foundVoice.getRawPointer(), 0, sizeof(AXVoice));
   foundVoice->index = voiceIndex;

   // Configure the voice with stuff we know about
   foundVoice->priority = priority;
   foundVoice->callbackEx = callback;
   foundVoice->userContext = userContext;

   auto extras = internal::getVoiceExtras(foundVoice->index);
   std::memset(extras.getRawPointer(), 0, sizeof(internal::AXVoiceExtras));
   extras->src.ratio = ufixed1616_t { 1.0 };

   // Save this to the acquired voice list so that it can be
   //  forcefully freed if a higher priority voice is needed.
   sAcquiredVoices.push_back(foundVoice);

   return foundVoice;
}

BOOL
AXCheckVoiceOffsets(virt_ptr<AXVoiceOffsets> offsets)
{
   return TRUE;
}

void
AXFreeVoice(virt_ptr<AXVoice> voice)
{
   auto voiceIter = std::find(sAcquiredVoices.begin(), sAcquiredVoices.end(), voice);
   decaf_check(voiceIter != sAcquiredVoices.end());

   // Erase this voice from the acquired list
   sAcquiredVoices.erase(voiceIter);

   // Make this voice available on the available stack!
   sAvailVoiceStack.push(voice);
}

uint32_t
AXGetMaxVoices()
{
   return AXMaxNumVoices;
}

uint32_t
AXGetVoiceCurrentOffsetEx(virt_ptr<AXVoice> voice,
                          virt_ptr<const void> samples)
{
   StackObject<AXVoiceOffsets> offsets;
   AXGetVoiceOffsetsEx(voice, offsets, samples);
   return offsets->currentOffset;
}

uint32_t
AXGetVoiceLoopCount(virt_ptr<AXVoice> voice)
{
   return sVoiceData->voiceExtras[voice->index].loopCount;
}

uint32_t
AXGetVoiceMixerSelect(virt_ptr<AXVoice> voice)
{
   return voice->renderer;
}

void
AXGetVoiceOffsetsEx(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceOffsets> offsets,
                    virt_ptr<const void> samples)
{
   voice->offsets.data = samples;
   AXGetVoiceOffsets(voice, offsets);
   voice->offsets = *offsets;
}

void
AXGetVoiceOffsets(virt_ptr<AXVoice> voice,
                  virt_ptr<AXVoiceOffsets> offsets)
{
   auto extras = internal::getVoiceExtras(voice->index);
   *offsets = voice->offsets;

   auto pageAddress = extras->data.memPageNumber << 29;
   auto baseAddress = virt_cast<virt_addr>(offsets->data);
   if (extras->data.format == AXVoiceFormat::ADPCM) {
      auto pageAddressSamples = static_cast<int64_t>(pageAddress) << 1;
      auto baseAddressSamples = static_cast<int64_t>(baseAddress) << 1;
      offsets->loopOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<int64_t>(extras->data.loopOffsetAbs) - baseAddressSamples);
      offsets->endOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<int64_t>(extras->data.endOffsetAbs) - baseAddressSamples);
      offsets->currentOffset = static_cast<uint32_t>(pageAddressSamples + static_cast<int64_t>(extras->data.currentOffsetAbs) - baseAddressSamples);
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      auto pageAddressSamples = pageAddress >> 1;
      auto baseAddressSamples = baseAddress >> 1;
      offsets->loopOffset = static_cast<uint32_t>(extras->data.loopOffsetAbs + pageAddressSamples - baseAddressSamples);
      offsets->endOffset = static_cast<uint32_t>(extras->data.endOffsetAbs + pageAddressSamples - baseAddressSamples);
      offsets->currentOffset = static_cast<uint32_t>(extras->data.currentOffsetAbs + pageAddressSamples - baseAddressSamples);
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      offsets->loopOffset = static_cast<uint32_t>(extras->data.loopOffsetAbs + pageAddress - baseAddress);
      offsets->endOffset = static_cast<uint32_t>(extras->data.endOffsetAbs + pageAddress - baseAddress);
      offsets->currentOffset = static_cast<uint32_t>(extras->data.currentOffsetAbs + pageAddress - baseAddress);
   } else {
      decaf_abort(fmt::format("Unexpected voice data format {}", extras->data.format))
   }
}

BOOL
AXIsVoiceRunning(virt_ptr<AXVoice> voice)
{
   auto extras = internal::getVoiceExtras(voice->index);
   return (extras->state != AXVoiceState::Stopped) ? 1 : 0;
}

void
AXSetVoiceAdpcm(virt_ptr<AXVoice> voice,
                virt_ptr<AXVoiceAdpcm> adpcm)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->adpcm = *adpcm;
   extras->syncBits |= internal::AXVoiceSyncBits::Adpcm;
}

void
AXSetVoiceAdpcmLoop(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceAdpcmLoopData> loopData)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->adpcmLoop = *loopData;
   extras->syncBits |= internal::AXVoiceSyncBits::AdpcmLoop;
}

void
AXSetVoiceCurrentOffset(virt_ptr<AXVoice> voice,
                        uint32_t offset)
{
   auto extras = internal::getVoiceExtras(voice->index);
   auto baseAddress = virt_cast<virt_addr>(voice->offsets.data) & 0x1FFFFFFF;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.currentOffsetAbs = (baseAddress << 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.currentOffsetAbs = (baseAddress >> 1) + offset;
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.currentOffsetAbs = baseAddress + offset;
   } else {
      decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::CurrentOffset;
}

void
AXSetVoiceCurrentOffsetEx(virt_ptr<AXVoice> voice,
                          uint32_t offset,
                          virt_ptr<const void> samples)
{
   StackObject<AXVoiceOffsets> offsets;
   voice->offsets.data = samples;

   AXGetVoiceOffsets(voice, offsets);
   AXSetVoiceCurrentOffset(voice, offset);
}


AXResult
AXSetVoiceDeviceMix(virt_ptr<AXVoice> voice,
                    AXDeviceType type,
                    uint32_t deviceId,
                    virt_ptr<AXVoiceDeviceMixData> mixData)
{
   auto extras = internal::getVoiceExtras(voice->index);

   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId < AXNumTvDevices);

      for (auto c = 0; c < AXNumTvChannels; ++c) {
         for (auto b = 0; b < AXNumTvBus; ++b) {
            extras->tvVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->tvVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
      }

      break;
   case AXDeviceType::DRC:
      decaf_check(deviceId < AXNumDrcDevices);

      for (auto c = 0; c < AXNumDrcChannels; ++c) {
         for (auto b = 0; b < AXNumDrcBus; ++b) {
            extras->drcVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->drcVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
      }

      break;
   case AXDeviceType::RMT:
      decaf_check(deviceId < AXNumRmtDevices);

      for (auto c = 0; c < AXNumRmtChannels; ++c) {
         for (auto b = 0; b < AXNumRmtBus; ++b) {
            extras->rmtVolume[deviceId][c][b].volume = ufixed_1_15_t::from_data(mixData[c].bus[b].volume);
            extras->rmtVolume[deviceId][c][b].delta = ufixed_1_15_t::from_data(mixData[c].bus[b].delta);
         }
      }

      break;
   }

   return AXResult::Success;
}

void
AXSetVoiceEndOffset(virt_ptr<AXVoice> voice,
                    uint32_t offset)
{
   auto extras = internal::getVoiceExtras(voice->index);
   auto baseAddress = virt_cast<virt_addr>(voice->offsets.data) & 0x1FFFFFFF;
   voice->offsets.endOffset = offset;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.endOffsetAbs = virt_addr { (baseAddress << 1) + offset };
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.endOffsetAbs = virt_addr { (baseAddress >> 1) + offset };
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.endOffsetAbs = baseAddress + offset;
   } else {
      decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::EndOffset;
}

void
AXSetVoiceEndOffsetEx(virt_ptr<AXVoice> voice,
                      uint32_t offset,
                      virt_ptr<const void> samples)
{
   StackObject<AXVoiceOffsets> offsets;
   voice->offsets.data = samples;

   AXGetVoiceOffsets(voice, offsets);
   AXSetVoiceEndOffset(voice, offset);
}

AXResult
AXSetVoiceInitialTimeDelay(virt_ptr<AXVoice> voice,
                           uint16_t delay)
{
   if (AXIsVoiceRunning(voice)) {
      return AXResult::VoiceIsRunning;
   }

   if (delay > AXGetInputSamplesPerFrame()) {
      return AXResult::DelayTooBig;
   }

   auto extras = internal::getVoiceExtras(voice->index);
   extras->itdOn = uint16_t { 1 };
   extras->itdDelay = delay;
   extras->syncBits |= internal::AXVoiceSyncBits::Itd;
   voice->syncBits |= internal::AXVoiceSyncBits::Itd;

   return AXResult::Success;
}

void
AXSetVoiceLoopOffset(virt_ptr<AXVoice> voice,
                     uint32_t offset)
{
   auto extras = internal::getVoiceExtras(voice->index);
   auto baseAddress = virt_cast<virt_addr>(voice->offsets.data) & 0x1FFFFFFF;
   voice->offsets.loopOffset = offset;

   if (extras->data.format == AXVoiceFormat::ADPCM) {
      extras->data.loopOffsetAbs = virt_addr { (baseAddress << 1) + offset };
   } else if (extras->data.format == AXVoiceFormat::LPCM16) {
      extras->data.loopOffsetAbs = virt_addr { (baseAddress >> 1) + offset };
   } else if (extras->data.format == AXVoiceFormat::LPCM8) {
      extras->data.loopOffsetAbs = baseAddress + offset;
   } else {
      decaf_abort(fmt::format("Unexpected voice data type {}", extras->data.format));
   }

   extras->syncBits |= internal::AXVoiceSyncBits::LoopOffset;
}

void
AXSetVoiceLoopOffsetEx(virt_ptr<AXVoice> voice,
                       uint32_t offset,
                       virt_ptr<const void> samples)
{
   voice->offsets.data = samples;

   StackObject<AXVoiceOffsets> offsets;
   AXGetVoiceOffsets(voice, offsets);
   AXSetVoiceLoopOffset(voice, offset);
}

void
AXSetVoiceLoop(virt_ptr<AXVoice> voice,
               AXVoiceLoop loop)
{
   auto extras = internal::getVoiceExtras(voice->index);

   voice->offsets.loopingEnabled = loop;

   extras->data.loopFlag = loop;
   extras->syncBits |= internal::AXVoiceSyncBits::Loop;
}

uint32_t
AXSetVoiceMixerSelect(virt_ptr<AXVoice> voice,
                      uint32_t mixerSelect)
{
   auto oldMiderSelect = voice->renderer;
   decaf_warn_stub();
   voice->renderer = static_cast<AXRenderer>(mixerSelect);
   return oldMiderSelect;
}

void
AXSetVoiceOffsets(virt_ptr<AXVoice> voice,
                  virt_ptr<AXVoiceOffsets> offsets)
{
   decaf_check(offsets->data);

   voice->offsets = *offsets;

   internal::AXCafeVoiceData absOffsets;
   absOffsets.format = offsets->dataType;
   absOffsets.loopFlag = offsets->loopingEnabled;
   auto samples = virt_cast<virt_addr>(offsets->data);

   if (offsets->dataType == AXVoiceFormat::ADPCM) {
      absOffsets.loopOffsetAbs = ((samples << 1) + offsets->loopOffset) & 0x3fffffff;
      absOffsets.endOffsetAbs = ((samples << 1) + offsets->endOffset) & 0x3fffffff;
      absOffsets.currentOffsetAbs = ((samples << 1) + offsets->currentOffset) & 0x3fffffff;
      absOffsets.memPageNumber = static_cast<uint16_t>((samples + (offsets->currentOffset >> 1)) >> 29);
   } else if (offsets->dataType == AXVoiceFormat::LPCM16) {
      absOffsets.loopOffsetAbs = ((samples >> 1) + offsets->loopOffset) & 0x0fffffff;
      absOffsets.endOffsetAbs = ((samples >> 1) + offsets->endOffset) & 0x0fffffff;
      absOffsets.currentOffsetAbs = ((samples >> 1) + offsets->currentOffset) & 0x0fffffff;
      absOffsets.memPageNumber = static_cast<uint16_t>((samples + (offsets->currentOffset << 1)) >> 29);
   } else if (offsets->dataType == AXVoiceFormat::LPCM8) {
      absOffsets.loopOffsetAbs = (samples + offsets->loopOffset) & 0x1fffffff;
      absOffsets.endOffsetAbs = (samples + offsets->endOffset) & 0x1fffffff;
      absOffsets.currentOffsetAbs = (samples + offsets->currentOffset) & 0x1fffffff;
      absOffsets.memPageNumber = static_cast<uint16_t>((samples + offsets->currentOffset) >> 29);
   } else {
      decaf_abort(fmt::format("Unexpected voice data type {}", offsets->dataType));
   }

   internal::setVoiceAddresses(voice, absOffsets);
}

void
AXSetVoiceOffsetsEx(virt_ptr<AXVoice> voice,
                    virt_ptr<AXVoiceOffsets> offsets,
                    virt_ptr<void> samples)
{
   StackObject<AXVoiceOffsets> adjOffsets;
   *adjOffsets = *offsets;
   adjOffsets->data = samples;
   AXSetVoiceOffsets(voice, adjOffsets);
}

void
AXSetVoicePriority(virt_ptr<AXVoice> voice,
                   uint32_t priority)
{
   voice->priority = priority;
}

void
AXSetVoiceRmtOn(virt_ptr<AXVoice> voice,
                uint16_t on)
{
   decaf_warn_stub();
}

void
AXSetVoiceRmtIIRCoefs(virt_ptr<AXVoice> voice,
                      uint16_t filter,
                      var_args)
{
   decaf_warn_stub();
}

void
AXSetVoiceSrc(virt_ptr<AXVoice> voice,
              virt_ptr<AXVoiceSrc> src)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->src = *src;
   voice->syncBits |= internal::AXVoiceSyncBits::Src;
}

AXVoiceSrcRatioResult
AXSetVoiceSrcRatio(virt_ptr<AXVoice> voice,
                   float ratio)
{
   if (ratio < 0.0f) {
      return AXVoiceSrcRatioResult::RatioLessThanZero;
   }

   auto extras = internal::getVoiceExtras(voice->index);
   extras->src.ratio = ufixed1616_t { ratio };
   voice->syncBits |= internal::AXVoiceSyncBits::SrcRatio;

   return AXVoiceSrcRatioResult::Success;
}

void
AXSetVoiceSrcType(virt_ptr<AXVoice> voice,
                  AXVoiceSrcType type)
{
   auto extras = internal::getVoiceExtras(voice->index);

   if (type == AXVoiceSrcType::None) {
      extras->srcMode = uint16_t { 2 };
   } else if (type == AXVoiceSrcType::Linear) {
      extras->srcMode = uint16_t { 1 };
   } else if (type == AXVoiceSrcType::Unk0) {
      extras->srcMode = uint16_t { 0 };
      extras->srcModeUnk = uint16_t { 0 };
   } else if (type == AXVoiceSrcType::Unk1) {
      extras->srcMode = uint16_t { 0 };
      extras->srcModeUnk = uint16_t { 1 };
   } else if (type == AXVoiceSrcType::Unk2) {
      extras->srcMode = uint16_t { 0 };
      extras->srcModeUnk = uint16_t { 2 };
   }

   voice->syncBits |= internal::AXVoiceSyncBits::SrcType;
}

void
AXSetVoiceState(virt_ptr<AXVoice> voice,
                AXVoiceState state)
{
   auto extras = internal::getVoiceExtras(voice->index);
   if (voice->state != state) {
      extras->state = static_cast<uint16_t>(state);
      voice->state = state;
      voice->syncBits |= internal::AXVoiceSyncBits::State;
   }
}

void
AXSetVoiceType(virt_ptr<AXVoice> voice,
               AXVoiceType type)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->type = type;
   voice->syncBits |= internal::AXVoiceSyncBits::Type;
}

void
AXSetVoiceVe(virt_ptr<AXVoice> voice,
             virt_ptr<AXVoiceVeData> veData)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->ve = *veData;
   voice->syncBits |= internal::AXVoiceSyncBits::Ve;
}

void
AXSetVoiceVeDelta(virt_ptr<AXVoice> voice,
                  int16_t delta)
{
   auto extras = internal::getVoiceExtras(voice->index);
   if (extras->ve.delta != delta) {
      extras->ve.delta = delta;
      voice->syncBits |= internal::AXVoiceSyncBits::VeDelta;
   }
}

int32_t
AXVoiceBegin(virt_ptr<AXVoice> voice)
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return AXUserBegin();
}

int32_t
AXVoiceEnd(virt_ptr<AXVoice> voice)
{
   decaf_warn_stub();

   // TODO: Implement this properly
   return AXUserEnd();
}

BOOL
AXVoiceIsProtected(virt_ptr<AXVoice> voice)
{
   decaf_warn_stub();

   return FALSE;
}

namespace internal
{

void
initVoices()
{
   auto index = 0u;
   for (auto &voice : sVoiceData->voices) {
      voice.index = index++;
      sAvailVoiceStack.push(virt_addrof(voice));
   }
}

void
setVoiceAddresses(virt_ptr<AXVoice> voice,
                  AXCafeVoiceData &offsets)
{
   auto extras = internal::getVoiceExtras(voice->index);
   extras->data = offsets;

   if (offsets.format == AXVoiceFormat::ADPCM) {
      // Ensure offset not on ADPCM header
      decaf_check((offsets.loopOffsetAbs & 0xF) >= virt_addr { 2 });
      decaf_check((offsets.endOffsetAbs & 0xF) >= virt_addr { 2 });
      decaf_check((offsets.currentOffsetAbs & 0xF) >= virt_addr { 2 });
   } else if (offsets.format == AXVoiceFormat::LPCM8) {
      std::memset(virt_addrof(extras->adpcm).getRawPointer(), 0, sizeof(AXVoiceAdpcm));
      extras->adpcm.gain = uint16_t { 0x100 };
      voice->syncBits |= internal::AXVoiceSyncBits::Adpcm;
   } else if (offsets.format == AXVoiceFormat::LPCM16) {
      std::memset(virt_addrof(extras->adpcm).getRawPointer(), 0, sizeof(AXVoiceAdpcm));
      extras->adpcm.gain = uint16_t { 0x800 };
      voice->syncBits |= internal::AXVoiceSyncBits::Adpcm;
   } else {
      decaf_abort(fmt::format("Unexpected audio data format {}", offsets.format));
   }

   voice->syncBits &= ~(
      internal::AXVoiceSyncBits::Loop |
      internal::AXVoiceSyncBits::LoopOffset |
      internal::AXVoiceSyncBits::EndOffset |
      internal::AXVoiceSyncBits::CurrentOffset);
   voice->syncBits |= internal::AXVoiceSyncBits::Addr;
}

const std::vector<virt_ptr<AXVoice>>
getAcquiredVoices()
{
   return sAcquiredVoices;
}

virt_ptr<AXVoiceExtras>
getVoiceExtras(int index)
{
   decaf_check(index >= 0 && index < AXMaxNumVoices);
   return virt_addrof(sVoiceData->voiceExtras[index]);
}

} // namespace internal

void
Library::registerVoiceSymbols()
{
   RegisterFunctionExport(AXAcquireVoice);
   RegisterFunctionExport(AXAcquireVoiceEx);
   RegisterFunctionExport(AXCheckVoiceOffsets);
   RegisterFunctionExport(AXFreeVoice);
   RegisterFunctionExport(AXGetMaxVoices);
   RegisterFunctionExport(AXGetVoiceCurrentOffsetEx);
   RegisterFunctionExport(AXGetVoiceLoopCount);
   RegisterFunctionExport(AXGetVoiceMixerSelect);
   RegisterFunctionExport(AXGetVoiceOffsets);
   RegisterFunctionExport(AXGetVoiceOffsetsEx);
   RegisterFunctionExport(AXIsVoiceRunning);
   RegisterFunctionExport(AXSetVoiceAdpcm);
   RegisterFunctionExport(AXSetVoiceAdpcmLoop);
   RegisterFunctionExport(AXSetVoiceCurrentOffset);
   RegisterFunctionExport(AXSetVoiceDeviceMix);
   RegisterFunctionExport(AXSetVoiceEndOffset);
   RegisterFunctionExport(AXSetVoiceEndOffsetEx);
   RegisterFunctionExport(AXSetVoiceInitialTimeDelay);
   RegisterFunctionExport(AXSetVoiceLoopOffset);
   RegisterFunctionExport(AXSetVoiceLoopOffsetEx);
   RegisterFunctionExport(AXSetVoiceLoop);
   RegisterFunctionExport(AXSetVoiceMixerSelect);
   RegisterFunctionExport(AXSetVoiceOffsets);
   RegisterFunctionExport(AXSetVoiceOffsetsEx);
   RegisterFunctionExport(AXSetVoicePriority);
   RegisterFunctionExport(AXSetVoiceRmtOn);
   RegisterFunctionExport(AXSetVoiceRmtIIRCoefs);
   RegisterFunctionExport(AXSetVoiceSrc);
   RegisterFunctionExport(AXSetVoiceSrcType);
   RegisterFunctionExport(AXSetVoiceSrcRatio);
   RegisterFunctionExport(AXSetVoiceState);
   RegisterFunctionExport(AXSetVoiceType);
   RegisterFunctionExport(AXSetVoiceVe);
   RegisterFunctionExport(AXSetVoiceVeDelta);
   RegisterFunctionExport(AXVoiceBegin);
   RegisterFunctionExport(AXVoiceEnd);
   RegisterFunctionExport(AXVoiceIsProtected);

   RegisterDataInternal(sVoiceData);
}

} // namespace cafe::sndcore2
