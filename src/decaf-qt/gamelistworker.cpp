// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cstdlib>
#include <locale>
#include <fstream>
#include <sstream>

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QDirIterator>
#include <pugixml.hpp>
#include <common/platform_dir.h>

#include "gamelist.h"
#include "gamelistp.h"
#include "gamelistworker.h"


#ifdef _WIN32
#include <windows.h>
// windows.h needs to be included before other windows headers
#include <direct.h> // getcwd
#include <io.h>
#include <shellapi.h>
#include <shlobj.h> // for SHGetFolderPath
#include <tchar.h>
#endif

using DirectoryEntryCallable = std::function<bool(const QString &path,
                                                  const QString &iconPath,
                                                  const QString &title,
                                                  const QString &titleId,
                                                  const QString &publisher,
                                                  const QString &version)>;

namespace
{

QString
getXmlElement(const QString &path,
              const char *element,
              const char *attribute)
{
   auto doc = pugi::xml_document { };

   doc.load_file(path.toStdString().c_str());
   auto menu = doc.child(element);
   if (!menu) {
      return "";
   }

   auto child = menu.child(attribute);
   if (!child) {
      return "";
   }

   std::wstring str = pugi::as_wide(child.text().get());

   return QString::fromWCharArray(str.c_str()).replace("\n", " - ");
}

QList<QStandardItem*>
makeTitleListEntry(const QString &path,
                   const QString &iconPath,
                   const QString &title,
                   const QString &titleId, 
                   const QString &publisher,
                   const QString &version)
{
   // The title list uses this as compatibility number for untested titles
   QList<QStandardItem*> list
   {
      new TitleListItemPath(path, iconPath, title, publisher),
      new TitleListItem(version),
      new TitleListItem(titleId),
      new TitleListItem(path),
   };

   return list;
}
} // Anonymous namespace

TitleListWorker::TitleListWorker(QString dirPath,
                                 bool deepScan) :
   mDirPath(std::move(dirPath)), mDeepScan(deepScan)
{
}

bool
scanForTitles(QString &directory,
                      DirectoryEntryCallable callback)
{
   // Save the status of callback function
   bool callbackError = false;

   QDirIterator it(directory);
   while (it.hasNext()) {
      QFile f(it.next());

      QString fileName = it.fileName();

      if (fileName == "." || fileName == "..") {
         continue;
      }

      // For some reason pugu doesn't like strings converted from QStrings
      QString cosFileName = directory + "/" + fileName + "/code/cos.xml";
      QString appFileName = directory + "/" + fileName + "/code/app.xml";
      QString metaFileName = directory + "/" + fileName + "/meta/meta.xml";

      QString fullPath = directory + "/" + fileName;

      QString path = fullPath + "/code/" + getXmlElement(cosFileName, "app", "argstr");
      QString iconPath = fullPath + "/meta/iconTex.tga";
      QString title = getXmlElement(metaFileName, "menu", "longname_en");
      QString titleId = getXmlElement(metaFileName, "menu", "title_id");
      QString publisher = getXmlElement(metaFileName, "menu", "publisher_en");

      bool ok; uint appId = getXmlElement(appFileName, "app", "title_version").toUInt(&ok, 16);
      QString version;
      version.setNum(appId);

      if (!QFileInfo::exists(path)) {
         continue;
      }

      if (!callback(path, iconPath, title, titleId, publisher, version)) {
         callbackError = true;
         break;
      }
   } 
  

   if (callbackError) {
      return false;
   }

   return true;
}

void
TitleListWorker::addEntriesToTitleList(QString &dirPath,
                                       unsigned int recursion)
{
   const auto callback = [this, recursion](const QString &path,
                                           const QString &iconPath,
                                           const QString &title,
                                           const QString &titleId,
                                           const QString &publisher,
                                           const QString &version) -> bool
   {
      if (mStopProcessing) {
         // Breaks the callback loop.
         return false;
      }


      bool ok;
      int32_t hi = titleId.toLongLong(&ok, 16) >> 32;

      switch (hi & 0xFF) {
      case 0x00: // eShop title
      case 0x02: // eShop title demo / Kiosk Interactive Demo
      case 0x10: // System Application
      case 0x30: // Applet
         // We will show these types of title
         emit entryReady(makeTitleListEntry(path, iconPath, title, titleId, publisher, version));
         break;
      case 0xb: // Data";
      case 0xc: // eShop title DLC
      case 0xe: // eShop title update
      case 0x1b: // System Data Archive 
         // but not these guys;
         break;
      }

      return true;
   };

   scanForTitles(dirPath, callback);
}

void
TitleListWorker::run()
{
   mStopProcessing = false;
   mWatchList.append(mDirPath);
   addEntriesToTitleList(mDirPath, mDeepScan ? 256 : 0);
   emit finished(mWatchList);
}

void
TitleListWorker::cancel()
{
   this->disconnect();
   mStopProcessing = true;
}
