#include "decafinterface.h"
#include "inputdriver.h"
#include "sounddriver.h"
#include "settings.h"

#include <QCoreApplication>
#include <common/log.h>
#include <libconfig/config_toml.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_log.h>
#include <libdecaf/decaf_nullinputdriver.h>

DecafInterface::DecafInterface(SettingsStorage *settingsStorage,
                               InputDriver *inputDriver,
                               SoundDriver *soundDriver) :
   mInputDriver(inputDriver),
   mSettingsStorage(settingsStorage),
   mSoundDriver(soundDriver)
{
   QObject::connect(mSettingsStorage, &SettingsStorage::settingsChanged,
                    this, &DecafInterface::settingsChanged);
   settingsChanged();
}

void
DecafInterface::settingsChanged()
{
   auto settings = mSettingsStorage->get();
   decaf::setConfig(settings->decaf);
   gpu::setConfig(settings->gpu);
   cpu::setConfig(settings->cpu);
}

void
DecafInterface::startLogging()
{
   decaf::initialiseLogging("decaf-qt");
}

void
DecafInterface::startGame(QString path)
{
   mStarted = true;

   decaf::addEventListener(this);
   decaf::setInputDriver(mInputDriver);
   decaf::setSoundDriver(mSoundDriver);
   gLog->info("Config path {}", mSettingsStorage->path());

   decaf::initialise(path.toLocal8Bit().constData());
   decaf::start();
}

void
DecafInterface::shutdown()
{
   if (mStarted) {
      decaf::shutdown();
      mStarted = false;
   }
}

void
DecafInterface::onGameLoaded(const decaf::GameInfo &info)
{
   emit titleLoaded(info.titleId, QString::fromStdString(info.executable));
}
