#include "decaf.h"
#include "input.h"

namespace input
{

void
sampleVpadController(int channel, vpad::Status &status)
{
   decaf::getInputDriver()->sampleVpadController(channel, status);
}

void
sampleWpadController(int channel, wpad::Status &status)
{
   decaf::getInputDriver()->sampleWpadController(channel, status);
}

} // namespace input
