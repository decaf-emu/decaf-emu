#include "decafsdl_config.h"
#include "decafsdl_sound.h"

#include <common/decaf_assert.h>
#include <SDL.h>
#include <libdecaf/decaf_log.h>

DecafSDLSound::DecafSDLSound()
{
   // Initialise logger
   mLog = decaf::makeLogger("sdl-sound");
}

DecafSDLSound::~DecafSDLSound()
{
   SDL_CloseAudio();
}

bool
DecafSDLSound::start(unsigned outputRate,
                     unsigned numChannels)
{
   mNumChannelsIn = numChannels;
   mNumChannelsOut = std::min(numChannels, 2u);  // TODO: support surround output
   mOutputFrameLen = config::sound::frame_length * (outputRate / 1000);

   // Set up the ring buffer with enough space for 3 output frames of audio
   mOutputBuffer.resize(mOutputFrameLen * mNumChannelsOut * 3);
   mBufferWritePos = 0;
   mBufferReadPos = 0;

   SDL_AudioSpec audiospec;
   audiospec.format = AUDIO_S16LSB;
   audiospec.freq = outputRate;
   audiospec.channels = static_cast<Uint8>(mNumChannelsOut);
   audiospec.samples = static_cast<Uint16>(mOutputFrameLen);
   audiospec.callback = sdlCallback;
   audiospec.userdata = this;

   if (SDL_OpenAudio(&audiospec, nullptr) != 0) {
      mLog->error("Failed to open audio device: {}", SDL_GetError());
      return false;
   }

   SDL_PauseAudio(0);
   return true;
}

void
DecafSDLSound::output(int16_t *samples, unsigned numSamples)
{
   // Discard channels from the input if necessary.
   if (mNumChannelsIn != mNumChannelsOut) {
      decaf_check(mNumChannelsOut < mNumChannelsIn);

      for (auto sample = 1u; sample < numSamples; ++sample) {
         for (auto channel = 0u; channel < mNumChannelsOut; channel++) {
            samples[sample * mNumChannelsOut + channel] = samples[sample * mNumChannelsIn + channel];
         }
      }
   }

   // Copy to the output buffer, ignoring the possibility of overrun
   //  (which should never happen anyway).
   auto numSamplesOut = static_cast<size_t>(numSamples * mNumChannelsOut);

   while (mBufferWritePos + numSamplesOut >= mOutputBuffer.size()) {
      auto samplesToCopy = mOutputBuffer.size() - mBufferWritePos;
      std::memcpy(&mOutputBuffer[mBufferWritePos], samples, samplesToCopy * 2);

      mBufferWritePos = 0;
      samples += samplesToCopy;
      numSamplesOut -= samplesToCopy;
   }

   std::memcpy(&mOutputBuffer[mBufferWritePos], samples, numSamplesOut * 2);
   mBufferWritePos += numSamplesOut;
}

void
DecafSDLSound::stop()
{
   SDL_CloseAudio();
}

void
DecafSDLSound::sdlCallback(void *instance_, Uint8 *stream_, int size)
{
   DecafSDLSound *instance = reinterpret_cast<DecafSDLSound *>(instance_);
   int16_t *stream = reinterpret_cast<int16_t *>(stream_);
   decaf_check(size >= 0);
   decaf_check(size % (2 * instance->mNumChannelsOut) == 0);
   auto numSamples = static_cast<size_t>(size) / 2;
   auto samplesAvail = (instance->mBufferWritePos + instance->mOutputBuffer.size() - instance->mBufferReadPos) % instance->mOutputBuffer.size();

   if (samplesAvail < numSamples) {
      // Rather than outputting the partial frame, output a full frame of
      //  silence to give audio generation a chance to catch up.
      std::memset(stream, 0, size);
   } else {
      decaf_check(instance->mBufferReadPos + numSamples <= instance->mOutputBuffer.size());
      std::memcpy(stream, &instance->mOutputBuffer[instance->mBufferReadPos], size);
      instance->mBufferReadPos = (instance->mBufferReadPos + numSamples) % instance->mOutputBuffer.size();
   }
}
