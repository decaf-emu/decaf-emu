#include "decaf_sound.h"

namespace decaf
{

SoundDriver *
sSoundDriver = nullptr;

void
setSoundDriver(SoundDriver *driver)
{
   sSoundDriver = driver;
}

SoundDriver *
getSoundDriver()
{
   return sSoundDriver;
}

} // namespace sound
