#include "decaf_nullinputdriver.h"

namespace decaf
{

using namespace input;

void NullInputDriver::sampleVpadController(int channel, input::vpad::Status &status)
{
   status.connected = false;
}

void NullInputDriver::sampleWpadController(int channel, input::wpad::Status &status)
{
   status.type = input::wpad::BaseControllerType::Disconnected;
}

} // namespace decaf
