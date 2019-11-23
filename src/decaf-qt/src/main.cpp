#include "mainwindow.h"
#include "decafinterface.h"
#include "inputdriver.h"
#include "sounddriver.h"

#include <libconfig/config_toml.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_input.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <libdecaf/decaf_log.h>

#include <SDL2/SDL.h>

#include <QApplication>
#include <QCommandLineParser>

#include <common/platform_debug.h>

void setupCommandLineOptions(QCommandLineParser *parser)
{
   parser->setApplicationDescription("decaf-qt");
   parser->addHelpOption();

   parser->addOption({ { "p", "play" }, "Loads the .RPX file specified in  <file name>.", "file" });
}

int main(int argc, char *argv[])
{
   QCoreApplication::setOrganizationName("decaf-emu");
   QCoreApplication::setOrganizationDomain("decaf-emu.com");
   QCoreApplication::setApplicationName("decaf-qt");

   auto app = QApplication { argc, argv };
   auto options = QCommandLineParser();

   setupCommandLineOptions(&options);

   options.process(app);

   decaf::createConfigDirectory();
   auto settings = SettingsStorage { decaf::makeConfigPath("config.toml") };

   auto inputDriver = InputDriver { &settings };
   auto soundDriver = SoundDriver { &settings };
   auto decafInterface = DecafInterface { &settings, &inputDriver, &soundDriver };

   auto mainWindow = MainWindow { &options, &settings, &decafInterface, &inputDriver };
   mainWindow.show();

   return app.exec();
}
