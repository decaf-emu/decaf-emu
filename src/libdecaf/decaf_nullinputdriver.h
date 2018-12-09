#pragma once
#include "decaf_input.h"

namespace decaf
{

class NullInputDriver : public InputDriver
{
public:
   virtual void sampleVpadController(int channel, input::vpad::Status &status) override;
   virtual void sampleWpadController(int channel, input::wpad::Status &status) override;
};

} // namespace decaf
