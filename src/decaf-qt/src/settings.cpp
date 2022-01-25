#include "settings.h"
#include "inputdriver.h"

#include <libconfig/config_toml.h>
#include <optional>
#include <toml++/toml.h>

static bool loadFromTOML(const toml::table &config,
                         InputConfiguration &inputConfiguration);
static bool saveToTOML(toml::table &config,
                       const InputConfiguration &inputConfiguration);

static bool loadFromTOML(const toml::table &config,
                         SoundSettings &soundSettings);
static bool saveToTOML(toml::table &config,
                       const SoundSettings &soundSettings);

static bool loadFromTOML(const toml::table &config,
                         UiSettings &uiSettings);
static bool saveToTOML(toml::table &config,
                       const UiSettings &uiSettings);

bool
loadSettings(const std::string &path,
             Settings &settings)
{
   try {
      toml::table toml = toml::parse_file(path);
      config::loadFromTOML(toml, settings.cpu);
      config::loadFromTOML(toml, settings.decaf);
      config::loadFromTOML(toml, settings.gpu);
      loadFromTOML(toml, settings.input);
      loadFromTOML(toml, settings.sound);
      loadFromTOML(toml, settings.ui);
      return true;
   } catch (const toml::parse_error &) {
      return false;
   }
}

bool
saveSettings(const std::string &path,
             const Settings &settings)
{
   toml::table toml;
   try {
      // Read current file and modify that
      toml = toml::parse_file(path);
   } catch (const toml::parse_error &) {
   }

   // Update it
   config::saveToTOML(toml, settings.decaf);
   config::saveToTOML(toml, settings.cpu);
   config::saveToTOML(toml, settings.gpu);
   saveToTOML(toml, settings.input);
   saveToTOML(toml, settings.sound);
   saveToTOML(toml, settings.ui);

   // Write to file
   std::ofstream out { path };
   if (!out.is_open()) {
      return false;
   }

   out << toml;
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
loadFromTOML(const toml::table &config,
             InputConfiguration &inputConfiguration)
{
   auto controllers = config.at_path("input.controller").as_array();
   if (!controllers) {
      return true;
   }

   for (const auto &controllerConfig : *controllers) {
      auto controllerConfigTable = controllerConfig.as_table();
      if (!controllerConfigTable) {
         continue;
      }
      auto &controller = inputConfiguration.controllers.emplace_back();

      auto controllerType = controllerConfigTable->get_as<std::string>("type");
      if (!controllerType) {
         continue;
      } else if (**controllerType == "gamepad") {
         controller.type = ControllerType::Gamepad;
      } else if (**controllerType == "wiimote") {
         controller.type = ControllerType::WiiMote;
      } else if (**controllerType == "pro") {
         controller.type = ControllerType::ProController;
      } else if (**controllerType == "classic") {
         controller.type = ControllerType::ClassicController;
      } else {
         continue;
      }

      auto readInputConfig =
         [](const toml::table &controllerConfig,
            InputConfiguration::Controller &controller,
            ButtonType buttonType)
         {
            auto &input = controller.inputs[static_cast<size_t>(buttonType)];
            auto buttonConfig = controllerConfig.get_as<toml::table>(getConfigButtonName(buttonType));
            if (buttonConfig) {
               if (auto guid = buttonConfig->get_as<std::string>("sdl_joystick_guid"); guid) {
                  input.joystickGuid = SDL_JoystickGetGUIDFromString(guid->get().c_str());
               }

               if (auto id = buttonConfig->get_as<int64_t>("sdl_joystick_duplicate_id"); id) {
                  input.joystickDuplicateId = id->get();
               }

               if (auto key = buttonConfig->get_as<int64_t>("key"); key) {
                  input.source = InputConfiguration::Input::KeyboardKey;
                  input.id = key->get();
               }

               if (auto button = buttonConfig->get_as<int64_t>("button"); button) {
                  input.source = InputConfiguration::Input::JoystickButton;
                  input.id = button->get();
               }

               if (auto axis = buttonConfig->get_as<int64_t>("axis"); axis) {
                  input.source = InputConfiguration::Input::JoystickAxis;
                  input.id = axis->get();
               }

               if (auto hat = buttonConfig->get_as<int64_t>("hat"); hat) {
                  input.source = InputConfiguration::Input::JoystickHat;
                  input.id = hat->get();
                  input.hatValue = 0;
               }

               if (auto hatValue = buttonConfig->get_as<int64_t>("hat_value"); hatValue) {
                  input.hatValue = hatValue->get();
               }

               if (auto invert = buttonConfig->get_as<bool>("invert"); invert) {
                  input.invert = invert->get();
               }
            }
         };

      for (auto i = 0u; i < static_cast<size_t>(ButtonType::MaxButtonType); ++i) {
         readInputConfig(*controllerConfigTable, controller, static_cast<ButtonType>(i));
      }
   }
   return true;
}

bool
saveToTOML(toml::table &config,
           const InputConfiguration &inputConfiguration)
{
   auto input = config.insert("input", toml::table()).first->second.as_table();

   auto controllers = toml::array();
   if (input->contains("controller")) {
      input->erase("controller");
   }

   for (auto &controller : inputConfiguration.controllers) {
      auto controllerConfig = toml::table();
      if (controller.type == ControllerType::Gamepad) {
         controllerConfig.insert_or_assign("type", "gamepad");
      } else if (controller.type == ControllerType::WiiMote) {
         controllerConfig.insert_or_assign("type", "wiimote");
      } else if (controller.type == ControllerType::ProController) {
         controllerConfig.insert_or_assign("type", "pro");
      } else if (controller.type == ControllerType::ClassicController) {
         controllerConfig.insert_or_assign("type", "classic");
      } else {
         continue;
      }

      for (auto i = 0u; i < static_cast<size_t>(ButtonType::MaxButtonType); ++i) {
         auto buttonType = static_cast<ButtonType>(i);
         auto &input = controller.inputs[i];
         auto inputConfig = toml::table();
         if (input.source == InputConfiguration::Input::Unassigned) {
            continue;
         } else if (input.source == InputConfiguration::Input::KeyboardKey) {
            inputConfig.insert_or_assign("key", input.id);
         } else if (input.source == InputConfiguration::Input::JoystickAxis) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig.insert_or_assign("sdl_joystick_guid", guidBuffer);
            inputConfig.insert_or_assign("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig.insert_or_assign("axis", input.id);
            inputConfig.insert_or_assign("invert", input.invert);
         } else if (input.source == InputConfiguration::Input::JoystickButton) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig.insert_or_assign("sdl_joystick_guid", guidBuffer);
            inputConfig.insert_or_assign("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig.insert_or_assign("button", input.id);
         } else if (input.source == InputConfiguration::Input::JoystickHat) {
            char guidBuffer[33];
            SDL_JoystickGetGUIDString(input.joystickGuid, guidBuffer, 33);
            inputConfig.insert_or_assign("sdl_joystick_guid", guidBuffer);
            inputConfig.insert_or_assign("sdl_joystick_duplicate_id", input.joystickDuplicateId);
            inputConfig.insert_or_assign("hat", input.id);
            inputConfig.insert_or_assign("hat_value", input.hatValue);
         }

         controllerConfig.insert_or_assign(getConfigButtonName(buttonType), inputConfig);
      }

      controllers.push_back(std::move(controllerConfig));
   }

   input->insert_or_assign("controller", std::move(controllers));
   return true;
}

bool
loadFromTOML(const toml::table &config,
             SoundSettings &soundSettings)
{
   config::readValue(config, "sound.playback_enabled", soundSettings.playbackEnabled);
   return true;
}

bool
saveToTOML(toml::table &config,
           const SoundSettings &soundSettings)
{
   auto sound = config.insert("sound", toml::table()).first->second.as_table();
   sound->insert_or_assign("playback_enabled", soundSettings.playbackEnabled);
   return true;
}


static const char *
translateTitleListMode(UiSettings::TitleListMode mode)
{
   if (mode == UiSettings::TitleListMode::TitleList) {
      return "list";
   } else if (mode == UiSettings::TitleListMode::TitleGrid) {
      return "grid";
   }

   return "";
}

static std::optional<UiSettings::TitleListMode>
translateTitleListMode(const std::string &text)
{
   if (text == "list") {
      return UiSettings::TitleListMode::TitleList;
   } else if (text == "grid") {
      return UiSettings::TitleListMode::TitleGrid;
   }

   return { };
}

bool
loadFromTOML(const toml::table &config,
             UiSettings &uiSettings)
{
   std::string titleListModeText;
   config::readValue(config, "ui.title_list_mode", titleListModeText);
   if (auto mode = translateTitleListMode(titleListModeText); mode) {
      uiSettings.titleListMode = *mode;
   }

   return true;
}

bool
saveToTOML(toml::table &config,
           const UiSettings &uiSettings)
{
   auto ui = config.insert("ui", toml::table()).first->second.as_table();

   if (auto text = translateTitleListMode(uiSettings.titleListMode); text) {
      ui->insert_or_assign("title_list_mode", text);
   }

   return true;
}
