#pragma once
#include <cstddef>

namespace input
{

enum class Category
{
   Invalid,
   VPAD,    // DRC
   WPAD,    // Wii Remote + extensions, Pro Controller, etc
   WBC,     // Balance board
};

namespace vpad
{

static const size_t MaxControllers = 1;

enum class Channel : size_t
{
   Drc0 = 0,
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
   StickL,
   StickR,
   Plus,
   Minus,
   Home,
   Sync,
};

} // namespace vpad

namespace wpad
{

static const size_t MaxControllers = 4;

enum class Channel : size_t
{
   Gamepad0 = 0,
   Gamepad1 = 1,
   Gamepad2 = 2,
   Gamepad3 = 3,
};

enum class Type
{
   Invalid,
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
   StickR,
   StickL,
};

} // namespace wpad

enum ButtonStatus
{
   ButtonReleased,
   ButtonPressed
};

bool
init();

ButtonStatus
getButtonStatus(vpad::Channel channel, vpad::Core button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Core button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Nunchuck button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Classic button);

ButtonStatus
getButtonStatus(wpad::Channel channel, wpad::Pro button);

} // namespace input
