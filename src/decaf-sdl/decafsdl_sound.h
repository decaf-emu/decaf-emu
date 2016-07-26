#pragma once
#include "libdecaf/decaf_sound.h"
#include <vector>
#include <SDL.h>

class DecafSDLSound : public decaf::SoundDriver
{
public:
   virtual ~DecafSDLSound()
   {
   }

   virtual bool
   start(int outputRate, int numChannels);

   virtual void
   output(int16_t *samples, int numSamples);

   virtual void
   stop();

private:
   int mNumChannelsIn;  // Number of channels of data we receive in output()
   int mNumChannelsOut; // Number of channels we send to the audio device
   int mOutputFrameLen; // Number of samples (per channel) in an output frame

   // Output buffer (ring buffer): written by output(), read by SDL callback
   std::vector<int16_t> mOutputBuffer;
   size_t mBufferWritePos; // Index of next sample (array element) to write
   size_t mBufferReadPos;  // Index of next sample (array element) to read

   static void
   sdlCallback(void *instance_, Uint8 *stream_, int size);
};
