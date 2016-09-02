#include "snd_core.h"
#include "snd_core_core.h"
#include "snd_core_constants.h"
#include "snd_core_device.h"
#include "snd_core_voice.h"
#include "decaf_sound.h"
#include "ppcutils/stackobject.h"
#include "ppcutils/wfunc_call.h"
#include <array>
#include <common/fixed.h>

namespace snd_core
{

struct PpcCallbackData
{
   be_ptr<be_val<int32_t>> samplePtrs[6];
   be_val<int32_t> samples[6][144];

   AXAuxCallbackData auxCallbackData;
   AXDeviceFinalMixData finalMixCallbackData;
};

static const int
NumOutputSamples = 48000 * 3 / 1000;

static const ufixed_1_15_t
DefaultVolume = ufixed_1_15_t::from_data(0x8000);

static PpcCallbackData *
sCallbackData = nullptr;

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

static std::array<DeviceData, AXNumTvDevices>
gTvDevices;

static std::array<DeviceData, AXNumDrcDevices>
gDrcDevices;

static std::array<DeviceData, AXNumRmtDevices>
gRmtDevices;

namespace internal
{

static DeviceData *
getDevice(AXDeviceType type,
          uint32_t deviceId)
{
   switch (type) {
   case AXDeviceType::TV:
      decaf_check(deviceId < gTvDevices.size());
      return &gTvDevices[deviceId];
   case AXDeviceType::DRC:
      decaf_check(deviceId < gDrcDevices.size());
      return &gDrcDevices[deviceId];
   case AXDeviceType::RMT:
      decaf_check(deviceId < gRmtDevices.size());
      return &gRmtDevices[deviceId];
   default:
      return nullptr;
   }
}

template<typename Type>
static Type*
getMemPageAddress(uint32_t memPageNumber)
{
   // We have to do this this way due to the way that mem::translate handles
   //  nullptr's.  In the case of AX here, our memPageNumber can be 0, causing
   //  mem::translate to return 0, which is not what we want.
   return reinterpret_cast<Type*>(mem::base() + (memPageNumber << 29));
}

struct AudioDecoder
{
   // Basic information
   internal::AXCafeVoiceData offsets;
   AXVoiceType type;
   uint32_t loopCount;
   AXVoiceAdpcm adpcm;
   AXVoiceAdpcmLoopData adpcmLoop;
   bool isEof;

   void fromVoice(AXVoiceExtras *extras)
   {
      offsets = extras->data;
      type = extras->type;
      loopCount = extras->loopCount;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         adpcm = extras->adpcm;
         adpcmLoop = extras->adpcmLoop;
      }

      isEof = false;
   }

   void toVoice(AXVoiceExtras *extras)
   {
      extras->data = offsets;
      extras->loopCount = loopCount;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         extras->adpcm = adpcm;
      }
   }

   AudioDecoder& advance()
   {
      // Update prev sample
      auto sample = read();
      adpcm.prevSample[1] = adpcm.prevSample[0];
      adpcm.prevSample[0] = sample.data();

      if (offsets.currentOffsetAbs == offsets.endOffsetAbs) {
         if (offsets.loopFlag) {
            offsets.currentOffsetAbs = offsets.loopOffsetAbs;

            if (offsets.format == AXVoiceFormat::ADPCM && type != AXVoiceType::Streaming) {
               adpcm.predScale = adpcmLoop.predScale;
               adpcm.prevSample[0] = adpcmLoop.prevSample[0];
               adpcm.prevSample[1] = adpcmLoop.prevSample[1];
            }
         } else {
            decaf_check(!isEof);
            isEof = true;
         }

         loopCount++;
      } else {
         offsets.currentOffsetAbs += 1;

         // Read next header if were there
         if ((offsets.currentOffsetAbs & 0xf) < 2) {
            decaf_check((offsets.currentOffsetAbs & 0xf) == 0);

            auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);

            adpcm.predScale = data[offsets.currentOffsetAbs / 2];
            offsets.currentOffsetAbs += 2;
         }
      }

      return *this;
   }

   bool eof()
   {
      return isEof;
   }

   Pcm16Sample read()
   {
      decaf_check(!isEof);

      auto sampleIndex = offsets.currentOffsetAbs;

      if (offsets.format == AXVoiceFormat::ADPCM) {
         decaf_check((sampleIndex & 0xf) >= 2);

         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);

         auto coeffIndex = (adpcm.predScale.value() >> 4) & 7;
         auto scale = adpcm.predScale.value() & 0xF;
         auto yn1 = adpcm.prevSample[0].value();
         auto yn2 = adpcm.prevSample[1].value();
         auto coeff1 = adpcm.coefficients[coeffIndex * 2 + 0].value();
         auto coeff2 = adpcm.coefficients[coeffIndex * 2 + 1].value();

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
         auto adpcmSample = ((xn << 11) + 0x400 + (coeff1 * yn1) + (coeff2 * yn2)) >> 11;

         // Clamp the output
         auto clampedSample = std::min(std::max(adpcmSample, -32767), 32767);

         // Write to the output
         return Pcm16Sample::from_data(clampedSample);
      } else if (offsets.format == AXVoiceFormat::LPCM16) {
         auto data = getMemPageAddress<be_val<int16_t>>(offsets.memPageNumber);
         return Pcm16Sample::from_data(data[sampleIndex]);
      } else if (offsets.format == AXVoiceFormat::LPCM8) {
         auto data = getMemPageAddress<uint8_t>(offsets.memPageNumber);
         return Pcm16Sample::from_data(data[sampleIndex] << 8);
      } else {
         decaf_abort("Unexpected AXVoice data format");
      }
   }
};

void
sampleVoice(AXVoice *voice, Pcm16Sample *samples, int numSamples)
{
   static const auto FpOne = ufixed1616_t(1);
   static const auto FpZero = ufixed1616_t(0);

   memset(samples, 0, numSamples * sizeof(Pcm16Sample));

   auto extras = getVoiceExtras(voice->index);
   auto offsetFrac = ufixed1616_t(extras->src.currentOffsetFrac.value());

   AudioDecoder decoder;
   decoder.fromVoice(extras);

   for (auto i = 0; i < numSamples; ++i) {
      // Read in the current sample
      Pcm16Sample sample;
      if (offsetFrac == FpZero) {
         sample = decoder.read();
      } else {
         AudioDecoder nextDecoder(decoder);
         nextDecoder.advance();
         if (!nextDecoder.eof()) {
            sample = nextDecoder.read();
         } else {
            sample = 0;
         }
      }

      if (offsetFrac == FpZero) {
         samples[i] = sample;
      } else {
         auto thisSampleMul = FpOne - offsetFrac;
         auto lastSampleMul = offsetFrac;
         auto lastSample = Pcm16Sample::from_data(extras->src.lastSample[0]);
         samples[i] = sample * thisSampleMul + lastSample * lastSampleMul;
      }

      offsetFrac += extras->src.ratio;

      while (offsetFrac >= FpOne) {
         // Advance the voice by one sample
         offsetFrac -= FpOne;
         decoder.advance();

         // Update all the last sample listings.  Most of these are used
         //  for FFT resampling (which we don't currently handle).
         extras->src.lastSample[3] = extras->src.lastSample[2];
         extras->src.lastSample[2] = extras->src.lastSample[1];
         extras->src.lastSample[1] = extras->src.lastSample[0];
         extras->src.lastSample[0] = sample.data();

         // If we reached the end of the voice data, we should just leave
         if (decoder.eof()) {
            break;
         }

         // If we read through multiple samples due to a high SRC ratio,
         //  then we need to actually read the upcoming sample in expectation
         //  of the fact that its about to be stored in lastSample.
         if (offsetFrac >= FpOne) {
            sample = decoder.read();
         }
      }

      if (decoder.eof()) {
         break;
      }
   }

   if (decoder.eof()) {
      voice->state = AXVoiceState::Stopped;
   }

   decoder.toVoice(extras);

   extras->src.currentOffsetFrac = ufixed1616_t(offsetFrac);
}

void
decodeVoiceSamples(int numSamples)
{
   const auto voices = getAcquiredVoices();

   for (auto voice : voices) {
      auto extras = getVoiceExtras(voice->index);

      if (voice->state == AXVoiceState::Stopped) {
         extras->numSamples = 0;
         continue;
      }

      extras->numSamples = numSamples;
      sampleVoice(voice, extras->samples, numSamples);
   }

   // TODO: Apply Volume Evelope (ADSR)

   // TODO: Apply Biquad Filter

   // TODO: Apply Low Pass Filter
}

static Pcm16Sample gTvSamples[AXNumTvDevices][AXNumTvChannels][144];

void
invokeTvAuxCallback(uint32_t device, uint32_t auxId, uint32_t numSamples, Pcm16Sample samples[AXNumTvChannels][144])
{
   if (gTvDevices[device].aux[auxId].callback) {
      auto auxCbData = &sCallbackData->auxCallbackData;
      auxCbData->samples = numSamples;
      auxCbData->channels = AXNumTvChannels;

      for (auto ch = 0u; ch < AXNumTvChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            sCallbackData->samples[ch][i] = static_cast<int32_t>(samples[ch][i]);
         }
         sCallbackData->samplePtrs[ch] = &sCallbackData->samples[ch][0];
      }

      gTvDevices[device].aux[auxId].callback(
         sCallbackData->samplePtrs, gTvDevices[device].aux[auxId].userData, auxCbData);

      for (auto ch = 0u; ch < AXNumTvChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            samples[ch][i] = Pcm16Sample::from_data(sCallbackData->samples[ch][i]);
         }
      }
   }
}

void
invokeTvFinalMixCallback(uint32_t device, uint32_t numSamples, Pcm16Sample samples[AXNumTvChannels][144])
{
   if (gTvDevices[device].finalMixCallback) {
      auto mixCbData = &sCallbackData->finalMixCallbackData;
      mixCbData->channels = 6;
      mixCbData->samples = numSamples;
      mixCbData->unk1 = 1;
      mixCbData->channelsOut = mixCbData->channels;

      for (auto ch = 0u; ch < AXNumTvChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            sCallbackData->samples[ch][i] = static_cast<int32_t>(samples[ch][i]);
         }
         sCallbackData->samplePtrs[ch] = &sCallbackData->samples[ch][0];
      }
      mixCbData->data = sCallbackData->samplePtrs;

      gTvDevices[device].finalMixCallback(mixCbData);

      for (auto ch = 0u; ch < AXNumTvChannels; ++ch) {
         for (auto i = 0u; i < numSamples; ++i) {
            samples[ch][i] = Pcm16Sample::from_data(sCallbackData->samples[ch][i]);
         }
      }
   }
}

void
mixTvBus(uint32_t numSamples)
{
   decaf_check(numSamples == 96 || numSamples == 144);

   Pcm16Sample busSamples[AXNumTvDevices][AXNumTvBus][AXNumTvChannels][144];
   const auto voices = getAcquiredVoices();

   memset(busSamples, 0, sizeof(Pcm16Sample) * AXNumTvDevices * AXNumTvBus * AXNumTvChannels * 144);

   for (auto voice : voices) {
      auto extras = getVoiceExtras(voice->index);

      if (!extras->numSamples) {
         continue;
      }

      decaf_check(extras->numSamples == numSamples);

      for (auto device = 0u; device < AXNumTvDevices; ++device) {
         for (auto bus = 0u; bus < AXNumTvBus; ++bus) {
            for (auto channel = 0u; channel < AXNumTvChannels; ++channel) {
               auto &volume = extras->tvVolume[device][channel][bus];
               auto &out = busSamples[device][bus][channel];

               for (auto i = 0u; i < numSamples; ++i) {
                  out[i] += extras->samples[i] * volume.volume;
               }

               volume.volume += volume.delta;
            }
         }
      }
   }

   for (auto device = 0u; device < AXNumTvDevices; ++device) {
      for (auto bus = 1u; bus < AXNumTvBus; ++bus) {
         invokeTvAuxCallback(device, bus - 1, numSamples, busSamples[device][bus]);
      }
   }

   // Downmix all aux busses to main bus
   for (auto device = 0u; device < AXNumTvDevices; ++device) {
      for (auto bus = 1u; bus < AXNumTvBus; ++bus) {
         auto returnVolume = gTvDevices[device].aux[bus].returnVolume;

         for (auto channel = 0u; channel < AXNumTvChannels; ++channel) {
            auto mainBus = busSamples[device][0][channel];
            auto subBus = busSamples[device][bus][channel];

            for (auto i = 0u; i < numSamples; ++i) {
               mainBus[i] += subBus[i] * returnVolume;
            }
         }
      }
   }

   // Apply overall device volume
   for (auto device = 0u; device < AXNumTvDevices; ++device) {
      for (auto channel = 0u; channel < AXNumTvChannels; ++channel) {
         auto &mainBus = busSamples[device][0][channel];

         for (auto i = 0u; i < numSamples; ++i) {
            mainBus[i] = mainBus[i] * gTvDevices[0].volume;
         }
      }
   }

   // Now we need to perform upsampling (I think)
   auto upsample32to48 = [](Pcm16Sample *samples) {
      // currently lazy...
      auto output = samples;
      Pcm16Sample input[96];
      memcpy(input, output, sizeof(Pcm16Sample) * 96);

      // Perform upsampling
      for (auto i = 0u; i < NumOutputSamples; ++i) {
         float sampleIdx = static_cast<float>(i) / 144.0f * 96.0f;
         auto sampleLo = static_cast<uint32_t>(std::min(143.0f, std::floor(sampleIdx)));
         auto sampleHi = static_cast<uint32_t>(std::min(95.0f, std::ceil(sampleIdx)));
         float sampleFrac = sampleIdx - sampleLo;
         output[i] = (input[sampleLo] * (1.0f - sampleFrac)) + (input[sampleHi] * sampleFrac);
      }
   };

   // Perform upsampling and final mix callback invokation
   for (auto device = 0u; device < AXNumTvDevices; ++device) {
      if (gTvDevices[device].upsampleAfterFinalMix) {
         invokeTvFinalMixCallback(device, numSamples, busSamples[device][0]);

         if (numSamples != NumOutputSamples) {
            for (auto channel = 0u; channel < AXNumTvChannels; ++channel) {
               upsample32to48(busSamples[device][0][channel]);
            }
         }
      } else {
         if (numSamples != NumOutputSamples) {
            for (auto channel = 0u; channel < AXNumTvChannels; ++channel) {
               upsample32to48(busSamples[device][0][channel]);
            }
         }

         invokeTvFinalMixCallback(device, NumOutputSamples, busSamples[device][0]);
      }
   }

   // TODO: Apply compressor

   // TODO: Channel upmix/downmix, but I think we should let the audio driver (aka SDL) handle that

   for (auto device = 0u; device < AXNumTvDevices; ++device) {
      memcpy(&gTvSamples[device][0][0], &busSamples[device][0][0][0], sizeof(Pcm16Sample) * AXNumTvChannels * NumOutputSamples);
   }

}

void
mixOutput(int32_t *buffer, int numSamples, int numChannels)
{
   static const int NumOutputSamples = 48000 * 3 / 1000;

   decodeVoiceSamples(numSamples);
   mixTvBus(numSamples);

   // TODO: Mix DRC

   // TODO: Mix RMT

   for (auto i = 0; i < NumOutputSamples; ++i) {
      for (auto ch = 0; ch < numChannels; ++ch) {
         buffer[numChannels * i + ch] = gTvSamples[0][ch][i].data();
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
      *mode = gTvDevices[0].mode;
      break;
   case AXDeviceType::DRC:
      *mode = gDrcDevices[0].mode;
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
      for (auto &device : gTvDevices) {
         device.mode = mode;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.mode = mode;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.mode = mode;
      }
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
      *func= gTvDevices[0].finalMixCallback;
      break;
   case AXDeviceType::DRC:
      *func = gDrcDevices[0].finalMixCallback;
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
      for (auto &device : gTvDevices) {
         device.finalMixCallback = func;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.finalMixCallback = func;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.finalMixCallback = func;
      }
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
      for (auto &device : gTvDevices) {
         device.compressor = !!compressor;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.compressor = !!compressor;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.compressor = !!compressor;
      }
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
      *upsampleAfterFinalMixCallback = gTvDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
      break;
   case AXDeviceType::DRC:
      *upsampleAfterFinalMixCallback = gDrcDevices[0].upsampleAfterFinalMix ? TRUE : FALSE;
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
      for (auto &device : gTvDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
      break;
   case AXDeviceType::DRC:
      for (auto &device : gDrcDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
      break;
   case AXDeviceType::RMT:
      for (auto &device : gRmtDevices) {
         device.upsampleAfterFinalMix = !!upsampleAfterFinalMixCallback;
      }
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

   RegisterInternalData(sCallbackData);
}

} // namespace snd_core
