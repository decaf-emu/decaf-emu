#pragma once
#include "settings.h"

#include <QObject>
#include <QVector>

#include <array>
#include <libdecaf/decaf_input.h>
#include <mutex>
#include <cpptoml.h>

class QKeyEvent;
class SettingsStorage;

struct ConnectedJoystick
{
   SDL_Joystick *joystick = nullptr;
   SDL_JoystickGUID guid;
   SDL_JoystickID instanceId;

   // This is for when we have multiple controllers with the same guid
   int duplicateId = 0;
};

static inline bool operator ==(const SDL_JoystickGUID &lhs, const SDL_JoystickGUID &rhs)
{
   return memcmp(&lhs, &rhs, sizeof(SDL_JoystickGUID)) == 0;
}

class InputDriver : public QObject, public decaf::InputDriver
{
   Q_OBJECT

public:
   InputDriver(SettingsStorage *settingsStorage, QObject *parent = nullptr);
   ~InputDriver();

   void enableButtonEvents() { mButtonEventsEnabled = true; }
   void disableButtonEvents() { mButtonEventsEnabled = false; }

   void keyPressEvent(int key);
   void keyReleaseEvent(int key);
   void gamepadTouchEvent(bool down, float x, float y);

signals:
   void joystickConnected(SDL_JoystickID id, SDL_JoystickGUID guid, const char *name);
   void joystickDisconnected(SDL_JoystickID id, SDL_JoystickGUID guid);
   void joystickButtonDown(SDL_JoystickID id, SDL_JoystickGUID guid, int button);
   void joystickAxisMotion(SDL_JoystickID id, SDL_JoystickGUID guid, int axis, float value);
   void joystickHatMotion(SDL_JoystickID id, SDL_JoystickGUID guid, int hat, int value);
   void configurationUpdated();

private slots:
   void update();
   void settingsChanged();

private:
   void sampleVpadController(int channel, decaf::input::vpad::Status &status) override;
   void sampleWpadController(int channel, decaf::input::wpad::Status &status) override;

private:
   SettingsStorage *mSettingsStorage;

   bool mButtonEventsEnabled = false;
   std::mutex mConfigurationMutex;
   InputConfiguration mConfiguration;
   std::vector<ConnectedJoystick> mJoysticks;

   bool mTouchDown = false;
   float mTouchX = 0.0f;
   float mTouchY = 0.0f;
   std::array<bool, 0x100> mKeyboardState;
};
