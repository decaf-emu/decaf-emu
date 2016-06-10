#pragma once
#include <cstdint>

namespace decaf
{

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
   Plus,
   Minus,
   Home,
   Sync,
   LeftStick,
   LeftStickX,
   LeftStickY,
   RightStick,
   RightStickX,
   RightStickY
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

enum ButtonStatus : uint32_t
{
   ButtonReleased,
   ButtonPressed
};

} // namespace input

} // namespace decaf