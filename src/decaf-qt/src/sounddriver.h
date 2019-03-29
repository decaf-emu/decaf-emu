#pragma once
#include "settings.h"

#ifdef QT_MULTIMEDIA_LIB
#include <QAudioOutput>
#include <QObject>
#include <QDebug>

#include <libdecaf/decaf_sound.h>

class SoundDriver : public QObject, public decaf::SoundDriver
{
public:
   ~SoundDriver() override = default;

   bool start(unsigned outputRate, unsigned numChannels) override;
   void output(int16_t *samples, unsigned numSamples) override;
   void stop() override;

private:
   QAudioOutput *mAudioOutput;
   QIODevice *mAudioIo;
};
#else
#include <atomic>
#include <libdecaf/decaf_sound.h>
#include <SDL.h>
#include <spdlog/spdlog.h>
#include <vector>

#include <QObject>

class SettingsStorage;

class SoundDriver : public QObject, public decaf::SoundDriver
{
   Q_OBJECT

public:
   SoundDriver(SettingsStorage *settingsStorage);
   ~SoundDriver() override;

private:
   bool start(unsigned outputRate, unsigned numChannels) override;
   void output(int16_t *samples, unsigned numSamples) override;
   void stop() override;

private slots:
   void settingsChanged();

private:
   SettingsStorage *mSettingsStorage = nullptr;
   std::atomic<bool> mPlaybackEnabled { true };

   // Number of channels of data we receive in output()
   unsigned mNumChannelsIn = 0;

   // Number of channels we send to the audio device
   unsigned mNumChannelsOut = 0;

   // Number of samples (per channel) in an output frame
   unsigned mOutputFrameLen = 0;

   // Output buffer (ring buffer): written by output(), read by SDL callback
   std::vector<int16_t> mOutputBuffer = { };

   // Index of next sample (array element) to write
   size_t mBufferWritePos = 0u;

   // Index of next sample (array element) to read
   size_t mBufferReadPos = 0u;

   static void sdlCallback(void *instance, Uint8 *stream, int size);
};
#endif
