#include "sounddriver.h"

#ifdef QT_MULTIMEDIA_LIB
bool
SoundDriver::start(unsigned outputRate, unsigned numChannels)
{
   QAudioFormat format;
   format.setChannelCount(numChannels);
   format.setCodec("audio/pcm");
   format.setSampleRate(outputRate);
   format.setSampleSize(16);
   format.setSampleType(QAudioFormat::SignedInt);
   format.setByteOrder(QAudioFormat::LittleEndian);

   QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
   if (!info.isFormatSupported(format)) {
      qWarning() << "Raw audio format not supported by backend, cannot play audio.";
      return false;
   }

   mAudioOutput = new QAudioOutput(format, this);
   mAudioIo = mAudioOutput->start();
   return true;
}

void
SoundDriver::output(int16_t *samples, unsigned numSamples)
{
   mAudioIo->write(reinterpret_cast<const char *>(samples), numSamples * sizeof(int16_t));
}

void
SoundDriver::stop()
{
   mAudioOutput->stop();
   delete mAudioOutput;

   mAudioOutput = nullptr;
   mAudioIo = nullptr;
}
#else
#include <common/decaf_assert.h>
#include <SDL.h>
#include <libdecaf/decaf_log.h>

SoundDriver::SoundDriver(SettingsStorage *settingsStorage) :
   mSettingsStorage(settingsStorage)
{
   QObject::connect(mSettingsStorage, &SettingsStorage::settingsChanged,
                    this, &SoundDriver::settingsChanged);
   settingsChanged();
}

SoundDriver::~SoundDriver()
{
   SDL_CloseAudio();
}

void
SoundDriver::settingsChanged()
{
   auto settings = mSettingsStorage->get();
   if (mPlaybackEnabled != settings->sound.playbackEnabled) {
      mPlaybackEnabled = settings->sound.playbackEnabled;

      if (settings->sound.playbackEnabled) {
         SDL_PauseAudio(0);
      } else {
         SDL_PauseAudio(1);
      }
   }
}

bool
SoundDriver::start(unsigned outputRate,
                   unsigned numChannels)
{
   auto mFrameLength = 30; // TODO: Config
   mNumChannelsIn = numChannels;
   mNumChannelsOut = std::min(numChannels, 2u);  // TODO: support surround output
   mOutputFrameLen = mFrameLength * (outputRate / 1000);

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
      return false;
   }

   SDL_PauseAudio(0);
   return true;
}

void
SoundDriver::output(int16_t *samples, unsigned numSamples)
{
   if (!mPlaybackEnabled) {
      return;
   }

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
SoundDriver::stop()
{
   SDL_CloseAudio();
}

void
SoundDriver::sdlCallback(void *instance_, Uint8 *stream_, int size)
{
   SoundDriver *instance = reinterpret_cast<SoundDriver *>(instance_);
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
#endif
