#include "snd_core.h"
#include "snd_core_device.h"
#include "snd_core_voice.h"
#include "decaf_sound.h"
#include <array>
#include <common/fixed.h>

namespace snd_core
{

static const ufixed_1_15_t
DefaultVolume = ufixed_1_15_t::from_data(0x8000);

struct AuxData
{
   AXAuxCallback callback = nullptr;
   void *userData = nullptr;
   ufixed_1_15_t returnVolume = DefaultVolume;
};

struct DeviceData
{
   AXDeviceFinalMixCallback finalMixCallback = nullptr;
   std::array<AuxData, AXAuxId::Max> aux;
   bool linearUpsample = false;
   bool compressor = false;
   bool upsampleAfterFinalMix = false;
   ufixed_1_15_t volume = DefaultVolume;
   AXDeviceMode mode;
};

static DeviceData
gTvDevice;

static DeviceData
gDrcDevice;

static std::array<DeviceData, 4>
gRmtDevices;

namespace internal
{

static DeviceData *
getDevice(AXDeviceType type,
          uint32_t deviceId)
{
   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId == 0);
      return &gTvDevice;
   case AXDeviceType::DRC:
      decaf_check(deviceId == 0);
      return &gDrcDevice;
   case AXDeviceType::RMT:
      decaf_check(deviceId < gRmtDevices.size());
      return &gRmtDevices[deviceId];
   default:
      return nullptr;
   }
}

static int32_t
nextSampleAdpcm(AXVoice *voice,
                AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<const uint8_t *>(voice->offsets.data.get());

   if (voice->offsets.currentOffset % 16 == 0) {
      extras->adpcmPredScale = data[voice->offsets.currentOffset / 2];
      voice->offsets.currentOffset += 2;
   }

   auto sampleIndex = voice->offsets.currentOffset++;
   auto coeffIndex = (extras->adpcmPredScale >> 4) & 7;
   auto scale = extras->adpcmPredScale & 0xF;
   auto yn1 = extras->adpcmPrevSample[0];
   auto yn2 = extras->adpcmPrevSample[1];
   auto coeff1 = extras->adpcmCoeff[coeffIndex * 2];
   auto coeff2 = extras->adpcmCoeff[coeffIndex * 2 + 1];

   // Extract the 4-bit signed sample from the appropriate byte
   int sampleData = data[sampleIndex / 2];

   if (sampleIndex % 2 == 0) {
      sampleData &= 0xF;
   } else {
      sampleData >>= 4;
   }

   if (sampleData >= 8) {
      sampleData -= 16;
   }

   // Calculate sample
   auto xn = sampleData << scale;
   auto sample = ((xn << 11) + 0x400 + coeff1 * yn1 + coeff2 * yn2) >> 11;

   // Clamp to output range
   sample = std::min(std::max(sample, -32768), 32767);

   // Update prev sample
   extras->adpcmPrevSample[0] = sample;
   extras->adpcmPrevSample[1] = extras->adpcmPrevSample[0];
   return sample;
}

static int32_t
nextSampleLpcm16(AXVoice *voice,
                 AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<const be_val<int16_t> *>(voice->offsets.data.get());
   return data[voice->offsets.currentOffset++];
}

static int32_t
nextSampleLpcm8(AXVoice *voice,
                AXVoiceExtras *extras)
{
   auto data = reinterpret_cast<const uint8_t *>(voice->offsets.data.get());
   return (data[voice->offsets.currentOffset++] - 128) << 8;
}

// Returns false at end of stream (if not looped), else true
static bool
readNextSample(AXVoice *voice,
               AXVoiceExtras *extras)
{
   int32_t sample = 0;
   switch (voice->offsets.dataType) {
   case AXVoiceFormat::ADPCM:
      sample = nextSampleAdpcm(voice, extras);
      break;
   case AXVoiceFormat::LPCM16:
      sample = nextSampleLpcm16(voice, extras);
      break;
   case AXVoiceFormat::LPCM8:
      sample = nextSampleLpcm8(voice, extras);
      break;
   }

   extras->prevSample = extras->currentSample;
   extras->currentSample = sample;

   if (voice->offsets.currentOffset > voice->offsets.endOffset) {
      if (voice->offsets.loopingEnabled) {
         voice->offsets.currentOffset = voice->offsets.loopOffset;
         if (voice->offsets.dataType == AXVoiceFormat::ADPCM) {
            extras->adpcmPredScale = extras->adpcmLoopPredScale;
            extras->adpcmPrevSample[0] = extras->adpcmLoopPrevSample[0];
            extras->adpcmPrevSample[1] = extras->adpcmLoopPrevSample[1];
         }
         extras->loopCount++;
      } else {
         return false;
      }
   }

   return true;
}

void
mixOutput(int32_t *buffer, int numSamples, int numChannels)
{
   memset(buffer, 0, sizeof(*buffer) * numSamples * numChannels);

   const auto voices = getAcquiredVoices();

   for (auto voice : voices) {
      if (voice->state == AXVoiceState::Stopped) {
         continue;
      }

      auto extras = getVoiceExtras(voice->index);

      // TODO: This is all very inefficient at the moment.
      //  Plenty of room for improvement.
      for (auto i = 0; i < numSamples; ++i) {
         while (extras->offsetFrac >= 0) {
            extras->offsetFrac -= 0x10000;
            if (!readNextSample(voice, extras)) {
               voice->state = AXVoiceState::Stopped;
               break;
            }
         }

         auto weight = extras->offsetFrac + 0x10000;
         int32_t sample;

         if (weight == 0) {
            sample = extras->currentSample;
         } else {
            int32_t weightedPrev = extras->prevSample * (0x10000 - weight);
            int32_t weightedCurrent = extras->currentSample * weight;
            sample = ((weightedPrev + weightedCurrent) >> 16);
         }

         // TODO: figure out the "full volume" value
         const auto fullVolume = 0x8000;

         for (auto ch = 0; ch < numChannels; ++ch) {
            buffer[numChannels*i+ch] += sample * extras->tvVolume[ch] / fullVolume;
         }

         extras->offsetFrac += extras->src.ratio.value().data();
      }
   }
}

} // namespace internal

AXResult
AXGetDeviceMode(AXDeviceType type,
                be_val<AXDeviceMode> *mode)
{
   if (!mode) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *mode = gTvDevice.mode;
      break;
   case AXDeviceType::DRC:
      *mode = gDrcDevice.mode;
      break;
   case AXDeviceType::RMT:
      *mode = gRmtDevices[0].mode;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceMode(AXDeviceType type,
                AXDeviceMode mode)
{
   switch (type) {
   case AXDeviceType::TV:
      gTvDevice.mode = mode;
      break;
   case AXDeviceType::DRC:
      gDrcDevice.mode = mode;
      break;
   case AXDeviceType::RMT:
      gRmtDevices[0].mode = mode;
      gRmtDevices[1].mode = mode;
      gRmtDevices[2].mode = mode;
      gRmtDevices[3].mode = mode;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type,
                            be_AXDeviceFinalMixCallback *func)
{
   if (!func) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *func= gTvDevice.finalMixCallback;
      break;
   case AXDeviceType::DRC:
      *func = gDrcDevice.finalMixCallback;
      break;
   case AXDeviceType::RMT:
      *func = gRmtDevices[0].finalMixCallback;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type,
                                 AXDeviceFinalMixCallback func)
{
   switch (type) {
   case AXDeviceType::TV:
      gTvDevice.finalMixCallback = func;
      break;
   case AXDeviceType::DRC:
      gDrcDevice.finalMixCallback = func;
      break;
   case AXDeviceType::RMT:
      gRmtDevices[0].finalMixCallback = func;
      gRmtDevices[1].finalMixCallback = func;
      gRmtDevices[2].finalMixCallback = func;
      gRmtDevices[3].finalMixCallback = func;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetAuxCallback(AXDeviceType type,
                 uint32_t deviceId,
                 AXAuxId auxId,
                 be_AXAuxCallback *callback,
                 be_ptr<void> *userData)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (callback) {
      *callback = device->aux[auxId].callback;
   }

   if (userData) {
      *userData = device->aux[auxId].userData;
   }

   return AXResult::Success;
}

AXResult
AXRegisterAuxCallback(AXDeviceType type,
                      uint32_t deviceId,
                      AXAuxId auxId,
                      AXAuxCallback callback,
                      void *userData)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->aux[auxId].callback = callback;
   device->aux[auxId].userData = userData;

   return AXResult::Success;
}

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type,
                           uint32_t deviceId,
                           BOOL linear)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->linearUpsample = !!linear;
   return AXResult::Success;
}

AXResult
AXSetDeviceCompressor(AXDeviceType type,
                      BOOL compressor)
{
   switch (type) {
   case AXDeviceType::TV:
      gTvDevice.compressor = !!compressor;
      break;
   case AXDeviceType::DRC:
      gDrcDevice.compressor = !!compressor;
      break;
   case AXDeviceType::RMT:
      gRmtDevices[0].compressor = !!compressor;
      gRmtDevices[1].compressor = !!compressor;
      gRmtDevices[2].compressor = !!compressor;
      gRmtDevices[3].compressor = !!compressor;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceUpsampleStage(AXDeviceType type,
                         BOOL *upsampleAfterFinalMixCallback)
{
   if (!upsampleAfterFinalMixCallback) {
      return AXResult::Success;
   }

   switch (type) {
   case AXDeviceType::TV:
      *upsampleAfterFinalMixCallback = gTvDevice.upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::DRC:
      *upsampleAfterFinalMixCallback = gDrcDevice.upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::RMT:
      *upsampleAfterFinalMixCallback = gRmtDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type,
                         BOOL upsampleAfterFinalMixCallback)
{
   switch (type) {
   case AXDeviceType::TV:
      gTvDevice.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   case AXDeviceType::DRC:
      gDrcDevice.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   case AXDeviceType::RMT:
      gRmtDevices[0].upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      gRmtDevices[1].upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      gRmtDevices[2].upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      gRmtDevices[3].upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      break;
   default:
      return AXResult::InvalidDeviceType;
   }

   return AXResult::Success;
}

AXResult
AXGetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  be_val<uint16_t> *volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (volume) {
      *volume = device->volume.data();
   }

   return AXResult::Success;
}

AXResult
AXSetDeviceVolume(AXDeviceType type,
                  uint32_t deviceId,
                  uint16_t volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->volume = ufixed_1_15_t::from_data(volume);
   return AXResult::Success;
}

AXResult
AXGetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     be_val<uint16_t> *volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   if (volume) {
      *volume = device->aux[auxId].returnVolume.data();
   }

   return AXResult::Success;
}

AXResult
AXSetAuxReturnVolume(AXDeviceType type,
                     uint32_t deviceId,
                     AXAuxId auxId,
                     uint16_t volume)
{
   auto device = internal::getDevice(type, deviceId);

   if (!device) {
      return AXResult::InvalidDeviceType;
   }

   device->aux[auxId].returnVolume = ufixed_1_15_t::from_data(volume);
   return AXResult::Success;
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXSetDeviceMode);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
   RegisterKernelFunction(AXSetDeviceLinearUpsampler);
   RegisterKernelFunction(AXSetDeviceCompressor);
   RegisterKernelFunction(AXGetDeviceUpsampleStage);
   RegisterKernelFunction(AXSetDeviceUpsampleStage);
   RegisterKernelFunction(AXGetDeviceVolume);
   RegisterKernelFunction(AXSetDeviceVolume);
   RegisterKernelFunction(AXGetAuxReturnVolume);
   RegisterKernelFunction(AXSetAuxReturnVolume);
}

} // namespace snd_core
