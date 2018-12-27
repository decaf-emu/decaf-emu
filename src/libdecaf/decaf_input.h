#pragma once
#include <cstddef>
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

struct ButtonStatus
{
   uint32_t sync : 1;
   uint32_t home : 1;
   uint32_t minus : 1;
   uint32_t plus : 1;
   uint32_t r : 1;
   uint32_t l : 1;
   uint32_t zr : 1;
   uint32_t zl : 1;
   uint32_t down : 1;
   uint32_t up : 1;
   uint32_t right : 1;
   uint32_t left : 1;
   uint32_t y : 1;
   uint32_t x : 1;
   uint32_t b : 1;
   uint32_t a : 1;
   uint32_t stickR : 1;
   uint32_t stickL : 1;
};

struct AccelerometerStatus
{
   float accelX;
   float accelY;
   float accelZ;
   float magnitute;
   float variation;
   float verticalX;
   float verticalY;
};

struct GyroStatus
{
   // TODO: Gyro status
};

struct MagnetometerStatus
{
   float x;
   float y;
   float z;
};

struct TouchStatus
{
   //! True if screen is currently being touched
   bool down;

   //! The x-coordinate of a touched point.
   int x;

   //! The y-coordinate of a touched point.
   int y;
};

struct Status
{
   //! Whether the controller is currently connected or not.
   bool connected;

   //! Indicates what buttons are held down.
   ButtonStatus buttons;

   //! Position of left analog stick
   float leftStickX;
   float leftStickY;

   //! Position of right analog stick
   float rightStickX;
   float rightStickY;

   //! Status of accelorometer
   AccelerometerStatus accelorometer;

   //! Status of gyro
   GyroStatus gyro;

   //! Current touch position on DRC
   TouchStatus touch;

   //! Status of DRC magnetometer
   MagnetometerStatus magnetometer;

   //! Current volume set by the slide control
   uint8_t slideVolume;

   //! Battery level of controller
   uint8_t battery;

   //! Status of DRC microphone
   uint8_t micStatus;
};

} // namespace vpad

namespace wpad
{

static const size_t
MaxChannels = 4;

enum class BaseControllerType
{
   Disconnected,
   Wiimote,
   Classic,
   Pro,
};

struct WiimoteButtonStatus
{
   uint32_t up : 1;
   uint32_t down : 1;
   uint32_t right : 1;
   uint32_t left : 1;
   uint32_t a : 1;
   uint32_t b : 1;
   uint32_t button1 : 1;
   uint32_t button2 : 1;
   uint32_t minus : 1;
   uint32_t home : 1;
   uint32_t plus : 1;
};

//! Wiimote
struct WiimoteStatus
{
   WiimoteButtonStatus buttons;
};

struct ClassicButtonStatus
{
   uint32_t up : 1;
   uint32_t down : 1;
   uint32_t right : 1;
   uint32_t left : 1;
   uint32_t a : 1;
   uint32_t b : 1;
   uint32_t x : 1;
   uint32_t y : 1;
   uint32_t r : 1;
   uint32_t l : 1;
   uint32_t zr : 1;
   uint32_t zl : 1;
   uint32_t plus : 1;
   uint32_t home : 1;
   uint32_t minus : 1;
};

//! Wii Classic Controller
struct ClassicStatus
{
   ClassicButtonStatus buttons;
   float leftStickX;
   float leftStickY;
   float rightStickX;
   float rightStickY;
   float triggerL;
   float triggerR;
};

struct ProButtonStatus
{
   uint32_t up : 1;
   uint32_t down : 1;
   uint32_t right : 1;
   uint32_t left : 1;
   uint32_t a : 1;
   uint32_t b : 1;
   uint32_t x : 1;
   uint32_t y : 1;
   uint32_t r : 1;
   uint32_t l : 1;
   uint32_t zr : 1;
   uint32_t zl : 1;
   uint32_t plus : 1;
   uint32_t home : 1;
   uint32_t minus : 1;
   uint32_t leftStick : 1;
   uint32_t rightStick : 1;
};

//! Wii U Pro Controller
struct ProStatus
{
   ProButtonStatus buttons;
   float leftStickX;
   float leftStickY;
   float rightStickX;
   float rightStickY;
};

struct NunchukButtonStatus
{
   uint32_t z : 1;
   uint32_t c : 1;
};

//! Wii Nunchuk extension
struct NunchukStatus
{
   bool connected;
   NunchukButtonStatus buttons;
   float accelX;
   float accelY;
   float accelZ;
   float stickX;
   float stickY;
};

//! Wii Motion Plus extension
struct MotionPlusStatus
{
   bool connected;
   float pitch;
   float yaw;
   float roll;
};

struct Status
{
   //! Base type of this controller, optionally a nunchuk or motion plus device
   //! can be connected on top of this base controller type.
   BaseControllerType type;

   union
   {
      WiimoteStatus wiimote;
      ProStatus pro;
      ClassicStatus classic;
   };

   MotionPlusStatus motionPlus;
   NunchukStatus nunchuk;
};

} // namespace wpad

} // namespace input

class InputDriver
{
public:
   virtual void sampleVpadController(int channel, input::vpad::Status &status) = 0;
   virtual void sampleWpadController(int channel, input::wpad::Status &status) = 0;
};

void
setInputDriver(InputDriver *driver);

InputDriver *
getInputDriver();

} // namespace decaf
