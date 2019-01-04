#pragma once
#include <cstdint>

namespace decaf
{

class SoundDriver
{
public:
   virtual ~SoundDriver()
   {
   }

   virtual bool
   start(unsigned outputRate,
         unsigned numChannels) = 0;

   // Sample data has channels interleaved.  The implementation may reuse
   //  the sample buffer as a temporary buffer (of length
   //  samples * numChannels * numSamples).
   // TODO: DRC/controller output not yet supported.
   virtual void
   output(int16_t *samples,
          unsigned numSamples) = 0;

   virtual void
   stop() = 0;
};

void setSoundDriver(SoundDriver *driver);
SoundDriver *getSoundDriver();

} // namespace decaf
