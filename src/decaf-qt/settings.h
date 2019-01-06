#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <SDL_joystick.h>

#include <QColor>
#include <QObject>

enum class ButtonType
{
   A,
   B,
   X,
   Y,
   R,
   L,
   ZR,
   ZL,
   Plus,
   Minus,
   Home,
   Sync,
   DpadUp,
   DpadDown,
   DpadLeft,
   DpadRight,
   LeftStickPress,
   LeftStickUp,
   LeftStickDown,
   LeftStickLeft,
   LeftStickRight,
   RightStickPress,
   RightStickUp,
   RightStickDown,
   RightStickLeft,
   RightStickRight,
   MaxButtonType,
};

enum class ControllerType
{
   Invalid,
   Gamepad,
   WiiMote,
   ProController,
   ClassicController,
};

struct InputConfiguration
{
   struct Input
   {
      enum Source
      {
         Unassigned,
         KeyboardKey,
         JoystickButton,
         JoystickAxis,
         JoystickHat,
      };

      Source source = Source::Unassigned;
      int id = -1;

      // Only valid for Source==Joystick*
      SDL_JoystickGUID joystickGuid;
      int joystickDuplicateId = 0; // For when there are multiple controllers with the same GUID

      // Only valid for Type==JoystickAxis
      bool invert = false; // TODO: Do we want to allow invert on everything?
      // TODO: deadzone? etc

      // Only valid for Source==JoystickHat
      int hatValue = -1;

      // Not loaded from config file, assigned on controller connect!
      SDL_Joystick *joystick = nullptr;
      SDL_JoystickID joystickInstanceId = -1;
   };

   struct Controller
   {
      ControllerType type = ControllerType::Invalid;
      std::array<Input, static_cast<size_t>(ButtonType::MaxButtonType)> inputs { };
   };

   std::vector<Controller> controllers;
   std::array<Controller *, 4> wpad = { nullptr, nullptr, nullptr, nullptr };
   std::array<Controller *, 2> vpad = { nullptr, nullptr };
};

struct DisplaySettings
{
   enum ViewMode
   {
      Split,
      TV,
      Gamepad1,
      Gamepad2,
   };

   ViewMode viewMode = ViewMode::Split;
   bool maintainAspectRatio = true;
   QColor backgroundColour = { 153, 51, 51 };
   double splitSeperation = 5.0;
};

struct SoundSettings
{
   //! Whether audio playback is enabled.
   bool playbackEnabled = false;
};

struct Settings
{
   decaf::Settings decaf = { };
   cpu::Settings cpu = { };
   gpu::Settings gpu = { };

   DisplaySettings display = { };
   InputConfiguration input = { };
   SoundSettings sound = { };
};

bool loadSettings(const std::string &path, Settings &settings);
bool saveSettings(const std::string &path, const Settings &settings);

class SettingsStorage : public QObject
{
   Q_OBJECT
public:
   SettingsStorage(std::string path) :
      mSettingsStorage(std::make_shared<Settings>()),
      mPath(std::move(path))
   {
      loadSettings(mPath, *mSettingsStorage);
   }

   std::string path()
   {
      std::lock_guard<std::mutex> lock { mMutex };
      return mPath;
   }

   std::shared_ptr<const Settings> get()
   {
      std::lock_guard<std::mutex> lock { mMutex };
      return mSettingsStorage;
   }

   void set(const Settings &settings)
   {
      mMutex.lock();
      mSettingsStorage = std::make_shared<Settings>(settings);
      saveSettings(mPath, settings);
      mMutex.unlock();

      emit settingsChanged();
   }

signals:
   void settingsChanged();

private:
   std::mutex mMutex;
   std::string mPath;
   std::shared_ptr<Settings> mSettingsStorage;
};
