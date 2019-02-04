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

   decaf::initialiseLogging("decaf-qt");
   decaf::addEventListener(this);
   decaf::setInputDriver(inputDriver);
   decaf::setSoundDriver(soundDriver);

   gLog->info("Config path {}", settingsStorage->path());
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
DecafInterface::startGame(QString path)
{
   mStarted = true;
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
