#pragma once
#include "decaf_input.h"

namespace decaf
{

class NullInputDriver : public InputDriver
{
public:
   // VPAD
   virtual input::vpad::Type
   getControllerType(input::vpad::Channel channel) override;

   virtual input::ButtonStatus
   getButtonStatus(input::vpad::Channel channel, input::vpad::Core button) override;

   virtual float
   getAxisValue(input::vpad::Channel channel, input::vpad::CoreAxis axis) override;

   virtual bool
   getTouchPosition(input::vpad::Channel channel, input::vpad::TouchPosition &position) override;

   // WPAD
   virtual input::wpad::Type
   getControllerType(input::wpad::Channel channel) override;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Core button) override;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Classic button) override;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Nunchuck button) override;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Pro button) override;

   virtual float
   getAxisValue(input::wpad::Channel channel, input::wpad::NunchuckAxis axis) override;

   virtual float
   getAxisValue(input::wpad::Channel channel, input::wpad::ProAxis axis) override;
};

} // namespace decaf
