#include "inputdriver.h"

#include <QKeyEvent>
#include <QTimer>
#include <SDL.h>
#include <SDL_gamecontroller.h>

InputDriver::InputDriver(SettingsStorage *settingsStorage,
                         QObject *parent) :
   QObject(parent),
   mSettingsStorage(settingsStorage)
{
   QObject::connect(mSettingsStorage, &SettingsStorage::settingsChanged,
                    this, &InputDriver::settingsChanged);
   settingsChanged();

   // Startup SDL
   SDL_Init(SDL_INIT_HAPTIC | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

   // Start a timer for polling for SDL events
   QTimer::singleShot(100, Qt::PreciseTimer, this, SLOT(update()));
}

InputDriver::~InputDriver()
{
   SDL_Quit();
}

static inline float
translateAxisValue(Sint16 value)
{
   if (value >= 0) {
      return value / static_cast<float>(SDL_JOYSTICK_AXIS_MAX);
   } else {
      return value / static_cast<float>(-(SDL_JOYSTICK_AXIS_MIN));
   }
}

void
InputDriver::update()
{
   SDL_Event event;

   while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_JOYDEVICEADDED:
      {
         auto joystick = SDL_JoystickOpen(event.jdevice.which);
         auto guid = SDL_JoystickGetGUID(joystick);
         auto instanceId = SDL_JoystickInstanceID(joystick);

         {
            std::unique_lock lock { mConfigurationMutex };
            auto connected = ConnectedJoystick { };
            connected.joystick = joystick;
            connected.guid = guid;
            connected.instanceId = instanceId;
            connected.duplicateId = 0;

            for (auto &others : mJoysticks) {
               if (others.guid == connected.guid) {
                  connected.duplicateId = std::max(connected.duplicateId, others.duplicateId + 1);
               }
            }

            for (auto &controller : mConfiguration.controllers) {
               for (auto &input : controller.inputs) {
                  if (input.joystickGuid == guid && input.joystickDuplicateId == connected.duplicateId) {
                     input.joystick = joystick;
                     input.joystickInstanceId = instanceId;
                  }
               }
            }

            mJoysticks.push_back(connected);
         }

         emit joystickConnected(instanceId, guid, SDL_JoystickName(joystick));
         break;
      }
      case SDL_JOYDEVICEREMOVED:
      {
         auto joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
         if (joystick) {
            joystickDisconnected(SDL_JoystickInstanceID(joystick), SDL_JoystickGetGUID(joystick));

            {
               std::unique_lock lock { mConfigurationMutex };
               for (auto itr = mJoysticks.begin(); itr != mJoysticks.end(); ++itr) {
                  if (itr->joystick == joystick) {
                     itr = mJoysticks.erase(itr);
                  }
               }

               for (auto &controller : mConfiguration.controllers) {
                  for (auto &input : controller.inputs) {
                     if (input.joystick == joystick) {
                        input.joystick = nullptr;
                     }
                  }
               }
            }

            SDL_JoystickClose(joystick);
         }
         break;
      }
      case SDL_JOYBUTTONDOWN:
      {
         if (mButtonEventsEnabled) {
            auto joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
            if (joystick) {
               joystickButtonDown(SDL_JoystickInstanceID(joystick), SDL_JoystickGetGUID(joystick), event.jbutton.button);
            }
         }
         break;
      }
      case SDL_JOYAXISMOTION:
      {
         if (mButtonEventsEnabled) {
            auto joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
            if (joystick) {
               joystickAxisMotion(SDL_JoystickInstanceID(joystick), SDL_JoystickGetGUID(joystick), event.jaxis.axis, translateAxisValue(event.jaxis.value));
            }
         }
         break;
      }
      case SDL_JOYHATMOTION:
      {
         if (mButtonEventsEnabled) {
            auto joystick = SDL_JoystickFromInstanceID(event.jdevice.which);
            if (joystick) {
               joystickHatMotion(SDL_JoystickInstanceID(joystick), SDL_JoystickGetGUID(joystick), event.jhat.hat, event.jhat.value);
            }
         }
         break;
      }

      // I don't think we actually care about game controllers?
      case SDL_CONTROLLERDEVICEADDED:
         qDebug("SDL_CONTROLLERDEVICEADDED");
         break;
      case SDL_CONTROLLERDEVICEREMAPPED:
         qDebug("SDL_CONTROLLERDEVICEREMAPPED");
         break;
      case SDL_CONTROLLERDEVICEREMOVED:
         qDebug("SDL_CONTROLLERDEVICEREMOVED");
         break;
      case SDL_CONTROLLERBUTTONDOWN:
         qDebug("SDL_CONTROLLERBUTTONDOWN");
         break;
      }
   }

   QTimer::singleShot(10, Qt::PreciseTimer, this, SLOT(update()));
}

const char *
getConfigButtonName(ButtonType type)
{
   switch (type) {
   case ButtonType::A:
      return "button_a";
   case ButtonType::B:
      return "button_b";
   case ButtonType::X:
      return "button_x";
   case ButtonType::Y:
      return "button_y";
   case ButtonType::R:
      return "button_r";
   case ButtonType::L:
      return "button_l";
   case ButtonType::ZR:
      return "button_zr";
   case ButtonType::ZL:
      return "button_zl";
   case ButtonType::Plus:
      return "button_plus";
   case ButtonType::Minus:
      return "button_minus";
   case ButtonType::Home:
      return "button_home";
   case ButtonType::Sync:
      return "button_sync";
   case ButtonType::DpadUp:
      return "dpad_up";
   case ButtonType::DpadDown:
      return "dpad_down";
   case ButtonType::DpadLeft:
      return "dpad_left";
   case ButtonType::DpadRight:
      return "dpad_right";
   case ButtonType::LeftStickPress:
      return "left_stick_press";
   case ButtonType::LeftStickUp:
      return "left_stick_up";
   case ButtonType::LeftStickDown:
      return "left_stick_down";
   case ButtonType::LeftStickLeft:
      return "left_stick_left";
   case ButtonType::LeftStickRight:
      return "left_stick_right";
   case ButtonType::RightStickPress:
      return "right_stick_press";
   case ButtonType::RightStickUp:
      return "right_stick_up";
   case ButtonType::RightStickDown:
      return "right_stick_down";
   case ButtonType::RightStickLeft:
      return "right_stick_left";
   case ButtonType::RightStickRight:
      return "right_stick_right";
   default:
      return "";
   }
}

void
InputDriver::keyPressEvent(int key)
{
   std::unique_lock<std::mutex> lock { mConfigurationMutex };
   mKeyboardState[key & 0xFF] = true;
}

void
InputDriver::keyReleaseEvent(int key)
{
   std::unique_lock<std::mutex> lock { mConfigurationMutex };
   mKeyboardState[key & 0xFF] = false;
}

void
InputDriver::gamepadTouchEvent(bool down, float x, float y)
{
   std::unique_lock<std::mutex> lock { mConfigurationMutex };
   mTouchDown = down;
   mTouchX = x;
   mTouchY = y;
}

void
InputDriver::settingsChanged()
{
   auto settings = mSettingsStorage->get();
   std::unique_lock<std::mutex> lock { mConfigurationMutex };
   mConfiguration = settings->input;

   // Map to correct SDL joystick
   for (auto &controller : mConfiguration.controllers) {
      for (auto &input : controller.inputs) {
         if (input.source == InputConfiguration::Input::JoystickAxis ||
             input.source == InputConfiguration::Input::JoystickButton ||
             input.source == InputConfiguration::Input::JoystickHat) {

            for (auto &connected : mJoysticks) {
               if (input.joystickInstanceId != -1) {
                  if (connected.instanceId == input.joystickInstanceId) {
                     input.joystick = connected.joystick;
                     input.joystickDuplicateId = connected.duplicateId;
                  }
               } else if (connected.guid == input.joystickGuid &&
                          connected.duplicateId == input.joystickDuplicateId) {
                  input.joystick = connected.joystick;
                  input.joystickInstanceId = connected.instanceId;
               }
            }
         }
      }
   }

   // TODO: Should probably allow a way to remember what VPAD / WPAD index is
   // assigned to each controller.

   // Update VPAD / WPAD assignment
   auto vpad = 0;
   auto wpad = 0;

   for (auto &controller : mConfiguration.controllers) {
      if (controller.type == ControllerType::Gamepad) {
         if (vpad < mConfiguration.vpad.size()) {
            mConfiguration.vpad[vpad++] = &controller;
         }
      } else if (controller.type != ControllerType::Invalid) {
         if (wpad < mConfiguration.wpad.size()) {
            mConfiguration.wpad[wpad++] = &controller;
         }
      }
   }
}

void
InputDriver::sampleVpadController(int channel, decaf::input::vpad::Status &status)
{
   std::unique_lock<std::mutex> lock { mConfigurationMutex };
   if (channel < 0 || channel > mConfiguration.vpad.size()) {
      status.connected = false;
      return;
   }

   auto getButtonState = [&](InputConfiguration::Controller *controller, ButtonType type) -> uint32_t
   {
      auto &input = controller->inputs[static_cast<size_t>(type)];
      if (input.source == InputConfiguration::Input::KeyboardKey) {
         if (input.id < 0) {
            return 0;
         }

         return !!mKeyboardState[input.id & 0xFF];
      } else if (input.source == InputConfiguration::Input::JoystickButton) {
         if (!input.joystick) {
            return 0;
         }

         return !!SDL_JoystickGetButton(input.joystick, input.id);
      } else if (input.source == InputConfiguration::Input::JoystickHat) {
         if (!input.joystick) {
            return 0;
         }

         return !!(SDL_JoystickGetHat(input.joystick, input.id) & input.hatValue);
      } else if (input.source == InputConfiguration::Input::JoystickAxis) {
         if (!input.joystick) {
            return 0;
         }

         // Button is considered pressed if axis value is >= 0.5f or <= -0.5f
         auto value = SDL_JoystickGetAxis(input.joystick, input.id);
         if (input.invert && value >= SDL_JOYSTICK_AXIS_MAX / 2) {
            return 1;
         } else if (!input.invert && value < SDL_JOYSTICK_AXIS_MIN / 2) {
            return 1;
         } else {
            return 0;
         }
      }

      return 0;
   };

   auto getAxisValue = [&](InputConfiguration::Controller *controller, ButtonType type) -> float
   {
      auto &input = controller->inputs[static_cast<size_t>(type)];
      if (input.source == InputConfiguration::Input::KeyboardKey) {
         if (input.id < 0) {
            return 0.0f;
         }

         return mKeyboardState[input.id & 0xFF] ? 1.0f : 0.0f;
      } else if (input.source == InputConfiguration::Input::JoystickButton) {
         if (!input.joystick) {
            return 0.0f;
         }

         return SDL_JoystickGetButton(input.joystick, input.id) ? 1.0f : 0.0f;
      } else if (input.source == InputConfiguration::Input::JoystickHat) {
         if (!input.joystick) {
            return 0.0f;
         }

         return (SDL_JoystickGetHat(input.joystick, input.id) & input.hatValue) ? 1.0f : 0.0f;
      } else if (input.source == InputConfiguration::Input::JoystickAxis) {
         if (!input.joystick) {
            return 0.0f;
         }

         auto value = SDL_JoystickGetAxis(input.joystick, input.id);
         if (!input.invert && value > 0) {
            return static_cast<float>(value) / static_cast<float>(SDL_JOYSTICK_AXIS_MAX);
         } else if (input.invert && value < 0) {
            return static_cast<float>(value) / static_cast<float>(SDL_JOYSTICK_AXIS_MIN);
         } else {
            return 0.0f;
         }
      }

      return 0.0f;
   };

   auto controller = mConfiguration.vpad[channel];

   if (!controller) {
      status.connected = false;
      return;
   }

   status.connected = true;
   status.buttons.sync = getButtonState(controller, ButtonType::Sync);
   status.buttons.home = getButtonState(controller, ButtonType::Home);
   status.buttons.minus = getButtonState(controller, ButtonType::Minus);
   status.buttons.plus = getButtonState(controller, ButtonType::Plus);
   status.buttons.r = getButtonState(controller, ButtonType::R);
   status.buttons.l = getButtonState(controller, ButtonType::L);
   status.buttons.zr = getButtonState(controller, ButtonType::ZR);
   status.buttons.zl = getButtonState(controller, ButtonType::ZL);
   status.buttons.down = getButtonState(controller, ButtonType::DpadDown);
   status.buttons.up = getButtonState(controller, ButtonType::DpadUp);
   status.buttons.right = getButtonState(controller, ButtonType::DpadRight);
   status.buttons.left = getButtonState(controller, ButtonType::DpadLeft);
   status.buttons.x = getButtonState(controller, ButtonType::X);
   status.buttons.y = getButtonState(controller, ButtonType::Y);
   status.buttons.a = getButtonState(controller, ButtonType::A);
   status.buttons.b = getButtonState(controller, ButtonType::B);
   status.buttons.stickR = getButtonState(controller, ButtonType::RightStickDown);
   status.buttons.stickL = getButtonState(controller, ButtonType::LeftStickDown);

   status.leftStickX = getAxisValue(controller, ButtonType::LeftStickRight);
   status.leftStickX -= getAxisValue(controller, ButtonType::LeftStickLeft);
   status.leftStickY = getAxisValue(controller, ButtonType::LeftStickUp);
   status.leftStickY -= getAxisValue(controller, ButtonType::LeftStickDown);

   status.rightStickX = getAxisValue(controller, ButtonType::RightStickRight);
   status.rightStickX -= getAxisValue(controller, ButtonType::RightStickLeft);
   status.rightStickY = getAxisValue(controller, ButtonType::RightStickUp);
   status.rightStickY -= getAxisValue(controller, ButtonType::RightStickDown);

   status.touch.down = mTouchDown;
   status.touch.x = mTouchX;
   status.touch.y = mTouchY;
}

void
InputDriver::sampleWpadController(int channel, decaf::input::wpad::Status &status)
{

}
