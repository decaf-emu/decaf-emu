#pragma once
#include "settings.h"

#include <libdecaf/decaf_eventlistener.h>
#include <libdecaf/decaf_graphics.h>
#include <libdecaf/decaf_sound.h>

#include <QObject>
#include <QString>

class InputDriver;
class SettingsStorage;
class SoundDriver;

class DecafInterface : public QObject, public decaf::EventListener
{
   Q_OBJECT

public:
   DecafInterface(SettingsStorage *settingsStorage, InputDriver *inputDriver,
                  SoundDriver *soundDriver);

public slots:
   void startGame(QString path);
   void shutdown();
   void settingsChanged();

signals:
   void titleLoaded(quint64 id, const QString &name);

protected:
   void onGameLoaded(const decaf::GameInfo &info) override;

private:
   bool mStarted = false;
   InputDriver *mInputDriver = nullptr;
   SettingsStorage *mSettingsStorage = nullptr;
   SoundDriver *mSoundDriver = nullptr;
};
