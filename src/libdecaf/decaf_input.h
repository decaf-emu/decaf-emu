#pragma once
#include <cstddef>
#include <cstdint>

namespace decaf
{

namespace input
{

enum class KeyboardAction
{
   Press,
   Release
};

enum class MouseButton
{
   Left,
   Right,
   Middle,
   Unknown
};

enum class MouseAction
{
   Press,
   Release
};

enum class KeyboardKey
{
   Unknown,

   // This is only used for ImgUi in the debugger, so only the keys
   //  which are actually used are included here for now.
   Tab,
   LeftArrow,
   RightArrow,
   UpArrow,
   DownArrow,
   PageUp,
   PageDown,
   Home,
   End,
   Delete,
   Backspace,
   Enter,
   Escape,
   LeftControl,
   LeftShift,
   LeftAlt,
   LeftSuper,
   RightControl,
   RightShift,
   RightAlt,
   RightSuper,
   A, B, C, D, E, F,
   G, H, I, J, K, L,
   M, N, O, P, Q, R,
   S, T, U, V, W, X,
   Y, Z,
   F1,  F2,  F3,  F4,  F5,  F6,
   F7,  F8,  F9,  F10, F11, F12
};

enum class Category
{
   Invalid,
   VPAD,    // DRC
   WPAD,    // Wii Remote + extensions, Pro Controller, etc
   WBC,     // Balance board
};

namespace vpad
{

static const size_t
MaxControllers = 1;

enum class Channel : size_t
{
   Gamepad0 = 0,
};

enum class Type
{
   Disconnected,
   DRC,
};

enum class Core
{
   Invalid,
   Up,
   Down,
   Left,
   Right,
   A,
   B,
   X,
   Y,
   TriggerR,
   TriggerL,
   TriggerZR,
   TriggerZL,
   Plus,
   Minus,
   Home,
   Sync,
   LeftStick,
   RightStick,
};

enum class CoreAxis
{
   LeftStickX,
   LeftStickY,
   RightStickX,
   RightStickY,
};

struct TouchPosition
{
   //! Normalised X position of input, from 0 to 1
   float x;

   //! Normalised Y position of touch input, from 0 to 1
   float y;
};

} // namespace vpad

namespace wpad
{

static const size_t
MaxControllers = 4;

enum class Channel : size_t
{
   Gamepad0 = 0,
   Gamepad1 = 1,
   Gamepad2 = 2,
   Gamepad3 = 3,
};

enum class Type
{
   Disconnected,
   WiiRemote,
   WiiRemoteNunchunk,
   Classic,
   Pro,
};

enum class Core
{
   Up,
   Down,
   Left,
   Right,
   A,
   B,
   Button1,
   Button2,
   Plus,
   Minus,
   Home
};

enum class Nunchuck
{
   Z,
   C,
};

enum class NunchuckAxis
{
   StickX,
   StickY,
};

enum class Classic
{
   Up,
   Down,
   Left,
   Right,
   A,
   B,
   X,
   Y,
   TriggerR,
   TriggerL,
   TriggerZR,
   TriggerZL,
   Plus,
   Home,
   Minus,
};

enum class Pro
{
   Up,
   Down,
   Left,
   Right,
   A,
   B,
   X,
   Y,
   TriggerR,
   TriggerL,
   TriggerZR,
   TriggerZL,
   Plus,
   Home,
   Minus,
   LeftStick,
   RightStick,
};

enum class ProAxis
{
   LeftStickX,
   LeftStickY,
   RightStickX,
   RightStickY,
};

} // namespace wpad

enum class ButtonStatus : uint32_t
{
   ButtonReleased,
   ButtonPressed
};

} // namespace input

class InputDriver
{
public:
   // VPAD
   virtual input::vpad::Type
   getControllerType(input::vpad::Channel channel) = 0;

   virtual input::ButtonStatus
   getButtonStatus(input::vpad::Channel channel, input::vpad::Core button) = 0;

   virtual float
   getAxisValue(input::vpad::Channel channel, input::vpad::CoreAxis axis) = 0;

   virtual bool
   getTouchPosition(input::vpad::Channel channel, input::vpad::TouchPosition &position) = 0;

   // WPAD
   virtual input::wpad::Type
   getControllerType(input::wpad::Channel channel) = 0;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Core button) = 0;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Classic button) = 0;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Nunchuck button) = 0;

   virtual input::ButtonStatus
   getButtonStatus(input::wpad::Channel channel, input::wpad::Pro button) = 0;

   virtual float
   getAxisValue(input::wpad::Channel channel, input::wpad::NunchuckAxis axis) = 0;

   virtual float
   getAxisValue(input::wpad::Channel channel, input::wpad::ProAxis axis) = 0;

   // TODO: Accelorometers, microphone, etc...
};

void
setInputDriver(InputDriver *driver);

InputDriver *
getInputDriver();

} // namespace decaf
