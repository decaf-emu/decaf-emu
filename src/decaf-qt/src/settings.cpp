#include "settings.h"
#include "inputdriver.h"

#include <libconfig/config_toml.h>
#include <optional>

static bool loadFromTOML(std::shared_ptr<cpptoml::table> config,
                         DisplaySettings &displaySettings);
static bool saveToTOML(std::shared_ptr<cpptoml::table> config,
                       const DisplaySettings &displaySettings);

static bool loadFromTOML(std::shared_ptr<cpptoml::table> config,
                         InputConfiguration &inputConfiguration);
static bool saveToTOML(std::shared_ptr<cpptoml::table> config,
                       const InputConfiguration &inputConfiguration);

static bool loadFromTOML(std::shared_ptr<cpptoml::table> config,
                         SoundSettings &soundSettings);
static bool saveToTOML(std::shared_ptr<cpptoml::table> config,
                       const SoundSettings &soundSettings);

bool
loadSettings(const std::string &path,
             Settings &settings)
{
   try {
      auto toml = cpptoml::parse_file(path);
      config::loadFromTOML(toml, settings.cpu);
      config::loadFromTOML(toml, settings.decaf);
      config::loadFromTOML(toml, settings.gpu);
      loadFromTOML(toml, settings.display);
      loadFromTOML(toml, settings.input);
      loadFromTOML(toml, settings.sound);
      return true;
   } catch (cpptoml::parse_exception ex) {
      return false;
   }
}

bool
saveSettings(const std::string &path,
             const Settings &settings)
{
   auto toml = std::shared_ptr<cpptoml::table> { };
   try {
      // Read current file and modify that
      toml = cpptoml::parse_file(path);
   } catch (cpptoml::parse_exception ex) {
      toml = cpptoml::make_table();
   }

   // Update it
   config::saveToTOML(toml, settings.decaf);
   config::saveToTOML(toml, settings.cpu);
   config::saveToTOML(toml, settings.gpu);
   saveToTOML(toml, settings.display);
   saveToTOML(toml, settings.input);
   saveToTOML(toml, settings.sound);

   // Write to file
   std::ofstream out { path };
   if (!out.is_open()) {
      return false;
   }

   out << (*toml);
   return true;
}

static const char *
translateViewMode(DisplaySettings::ViewMode mode)
{
   if (mode == DisplaySettings::ViewMode::TV) {
      return "tv";
   } else if (mode == DisplaySettings::ViewMode::Gamepad1) {
      return "gamepad1";
   } else if (mode == DisplaySettings::ViewMode::Gamepad2) {
      return "gamepad2";
   } else if (mode == DisplaySettings::ViewMode::Split) {
      return "split";
   }

   return nullptr;
}

static std::optional<DisplaySettings::ViewMode>
translateViewMode(const std::string &text)
{
   if (text == "tv") {
      return DisplaySettings::ViewMode::TV;
   } else if (text == "gamepad1") {
      return DisplaySettings::ViewMode::Gamepad1;
   } else if (text == "gamepad2") {
      return DisplaySettings::ViewMode::Gamepad2;
   } else if (text == "split") {
      return DisplaySettings::ViewMode::Split;
   }

   return { };
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             DisplaySettings &displaySettings)
{
   auto display = config->get_table("display");
   if (display) {
      if (auto viewModeText = display->get_as<std::string>("view_mode"); viewModeText) {
         if (auto viewMode = translateViewMode(*viewModeText); viewMode) {
            displaySettings.viewMode = *viewMode;
         }
      }

      if (auto maintainAspectRatio = display->get_as<bool>("maintain_aspect_ratio"); maintainAspectRatio) {
         displaySettings.maintainAspectRatio = *maintainAspectRatio;
      }

      if (auto splitSeperation = display->get_as<double>("split_seperation"); splitSeperation) {
         displaySettings.splitSeperation = *splitSeperation;
      }

      if (auto backgroundColour = display->get_array_of<int64_t>("background_colour");
          backgroundColour && backgroundColour->size() >= 3) {
         displaySettings.backgroundColour =
            QColor {
               static_cast<int>(backgroundColour->at(0)),
               static_cast<int>(backgroundColour->at(1)),
               static_cast<int>(backgroundColour->at(2))
            };
      }
   }

   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const DisplaySettings &displaySettings)
{
   auto display = config->get_table("display");
   if (!display) {
      display = cpptoml::make_table();
   }

   display->insert("view_mode", translateViewMode(displaySettings.viewMode));
   display->insert("maintain_aspect_ratio", displaySettings.maintainAspectRatio);
   display->insert("split_seperation", displaySettings.splitSeperation);

   auto backgroundColour = cpptoml::make_array();
   backgroundColour->push_back(displaySettings.backgroundColour.red());
   backgroundColour->push_back(displaySettings.backgroundColour.green());
   backgroundColour->push_back(displaySettings.backgroundColour.blue());
   display->insert("background_colour", backgroundColour);

   config->insert("display", display);
   return true;
}

static const char *
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

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             InputConfiguration &inputConfiguration)
{
   auto controllers = config->get_table_array_qualified("input.controller");

   if (controllers) {
      for (const auto &controllerConfig : *controllers) {
         auto controllerType = controllerConfig->get_as<std::string>("type").value_or("");
         auto &controller = inputConfiguration.controllers.emplace_back();

         if (controllerType == "gamepad") {
            controller.type = ControllerType::Gamepad;
         } else if (controllerType == "wiimote") {
            controller.type = ControllerType::WiiMote;
         } else if (controllerType == "pro") {
            controller.type = ControllerType::ProController;
         } else if (controllerType == "classic") {
            controller.type = ControllerType::ClassicController;
         } else {
            continue;
         }

         auto readInputConfig = [](std::shared_ptr<cpptoml::table> controllerConfig, InputConfiguration::Controller &controller, ButtonType buttonType)
         {
            auto &input = controller.inputs[static_cast<size_t>(buttonType)];
            auto buttonConfig = controllerConfig->get_table(getConfigButtonName(buttonType));
            if (buttonConfig) {
               if (auto guid = buttonConfig->get_as<std::string>("sdl_joystick_guid"); guid) {
                  input.joystickGuid = SDL_JoystickGetGUIDFromString(guid->c_str());
               }

               if (auto id = buttonConfig->get_as<int>("sdl_joystick_duplicate_id"); id) {
                  input.joystickDuplicateId = *id;
               }

               if (auto key = buttonConfig->get_as<int>("key"); key) {
                  input.source = InputConfiguration::Input::KeyboardKey;
                  input.id = *key;
               }

               if (auto button = buttonConfig->get_as<int>("button"); button) {
                  input.source = InputConfiguration::Input::JoystickButton;
                  input.id = *button;
               }

               if (auto axis = buttonConfig->get_as<int>("axis"); axis) {
                  input.source = InputConfiguration::Input::JoystickAxis;
                  input.id = *axis;
               }

               if (auto hat = buttonConfig->get_as<int>("hat"); hat) {
                  input.source = InputConfiguration::Input::JoystickHat;
                  input.id = *hat;
                  input.hatValue = 0;
               }

               if (auto hatValue = buttonConfig->get_as<int>("hat_value"); hatValue) {
                  input.hatValue = *hatValue;
               }

               if (auto invert = buttonConfig->get_as<bool>("invert"); invert) {
                  input.invert = *invert;
               }
            }
         };

         for (auto i = 0u; i < static_cast<size_t>(ButtonType::MaxButtonType); ++i) {
            readInputConfig(controllerConfig, controller, static_cast<ButtonType>(i));
         }
      }
   }

   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const InputConfiguration &inputConfiguration)
{
   auto input = config->get_table("input");
   if (!input) {
      input = cpptoml::make_table();
   }

   auto controllers = cpptoml::make_table_array();
   if (input->get_table_array("controller")) {
      input->erase("controller");
   }

   for (auto &controller : inputConfiguration.controllers) {
      auto controllerConfig = cpptoml::make_table();
      if (controller.type == ControllerType::Gamepad) {
         controllerConfig->insert("type", "gamepad");
      } else if (controller.type == ControllerType::WiiMote) {
         controllerConfig->insert("type", "wiimote");
      } else if (controller.type == ControllerType::ProController) {
         controllerConfig->insert("type", "pro");
      } else if (controller.type == ControllerType::ClassicController) {
         controllerConfig->insert("type", "classic");
      } else {
         continue;
      }

      for (auto i = 0u; i < static_cast<size_t>(ButtonType::MaxButtonType); ++i) {
         auto buttonType = static_cast<ButtonType>(i);
         auto &input = controller.inputs[i];
         auto inputConfig = cpptoml::make_table();
         if (input.source == InputConfiguration::Input::Unassigned) {
            continue;
         } else if (input.source == InputConfiguration::Input::KeyboardKey) {
            inputConfig->insert("key", input.id);
         } else if (input.source == InputConfiguration::Input::JoystickAxis) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig->insert("sdl_joystick_guid", guidBuffer);
            inputConfig->insert("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig->insert("axis", input.id);
            inputConfig->insert("invert", input.invert);
         } else if (input.source == InputConfiguration::Input::JoystickButton) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig->insert("sdl_joystick_guid", guidBuffer);
            inputConfig->insert("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig->insert("button", input.id);
         } else if (input.source == InputConfiguration::Input::JoystickHat) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig->insert("sdl_joystick_guid", guidBuffer);
            inputConfig->insert("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig->insert("hat", input.id);
            inputConfig->insert("hat_value", input.hatValue);
         }

         controllerConfig->insert(getConfigButtonName(buttonType), inputConfig);
      }

      controllers->push_back(controllerConfig);
   }

   input->insert("controller", controllers);
   config->insert("input", input);
   return true;
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             SoundSettings &soundSettings)
{
   if (auto sound = config->get_table("sound")) {
      if (auto playbackEnabled = sound->get_as<bool>("playback_enabled"); playbackEnabled) {
         soundSettings.playbackEnabled = *playbackEnabled;
      }
   }

   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const SoundSettings &soundSettings)
{
   auto sound = config->get_table("sound");
   if (!sound) {
      sound = cpptoml::make_table();
   }

   sound->insert("playback_enabled", soundSettings.playbackEnabled);
   config->insert("sound", sound);
   return true;
}
