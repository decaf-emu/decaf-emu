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
getXmlElement(const std::string &path,
              const char *element,
              const char *attribute)
{
   auto doc = pugi::xml_document { };

   doc.load_file(path.c_str(), 116u, pugi::xml_encoding::encoding_auto);
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
MakeTitleListEntry(const QString &path,
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

TitleListWorker::~TitleListWorker() = default;

std::u16string
UTF8ToUTF16(const std::string &input)
{
#ifdef _MSC_VER
   // Workaround for missing char16_t/char32_t instantiations in MSVC2017
   std::wstring_convert<std::codecvt_utf8_utf16<__int16>, __int16> convert;
   auto tmpBuffer = convert.from_bytes(input);
   return std::u16string(tmpBuffer.cbegin(), tmpBuffer.cend());
#else
   std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
   return convert.from_bytes(input);
#endif
}

#ifdef _WIN32
static std::wstring
CPToUTF16(uint32_t codePage,
          const std::string &input)
{
   const auto size = MultiByteToWideChar(codePage,
                                         0,
                                         input.data(),
                                         static_cast<int>(input.size()),
                                         nullptr,
                                         0);

   if (size == 0) {
      return L"";
   }

   std::wstring output(size, L'\0');

   if (size != MultiByteToWideChar(codePage, 0, input.data(), static_cast<int>(input.size()),
      &output[0], static_cast<int>(output.size()))) {
      output.clear();
   }

   return output;
}
#endif

std::wstring
UTF8ToUTF16W(const std::string &input)
{
   return CPToUTF16(CP_UTF8, input);
}

std::string
UTF16ToUTF8(const wchar_t *input,
            char dfault = '?',
            const std::locale &loc = std::locale())
{
   std::ostringstream stm;

   while (*input != L'\0') {
      stm << std::use_facet< std::ctype<wchar_t> >(loc).narrow(*input++, dfault);
   }
   return stm.str();
}

bool
ForeachDirectoryEntry(const std::string &directory,
                      DirectoryEntryCallable callback)
{
   // Save the status of callback function
   bool callbackError = false;

#ifdef _WIN32
   // Find the first file in the directory.
   WIN32_FIND_DATAW ffd;

   HANDLE handleFind = FindFirstFileW(UTF8ToUTF16W(directory + "\\*").c_str(), &ffd);
   if (handleFind == INVALID_HANDLE_VALUE) {
      FindClose(handleFind);
      return false;
   }
   // windows loop
   do {
      const std::string virtualName(UTF16ToUTF8(ffd.cFileName));
      QString fileName = QString::fromWCharArray(ffd.cFileName);
#else
      DIR *dirp = opendir(directory.c_str());
      if (!dirp) {
         return false;
      }

      // non windows loop
      while (struct dirent *result = readdir(dirp)) {
         const std::string virtual_name(result->d_name);
         QString fileName = QString::fromStdString(virtual_name);
#endif

      if (virtualName == "." || virtualName == "..") {
         continue;
      }

      // For some reason pugu doesn't like strings converted from QStrings
      std::string cosFileName = directory + "/" + virtualName + "/code/cos.xml";
      std::string appFileName = directory + "/" + virtualName + "/code/app.xml";
      std::string metaFileName = directory + "/" + virtualName + "/meta/meta.xml";

      QString fullPath = QString::fromStdString(directory) + "/" + fileName;

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

   #ifdef _WIN32
   } while (FindNextFileW(handleFind, &ffd) != 0);
   FindClose(handleFind);
   #else
   }
   closedir(dirp);
   #endif

   if (callbackError) {
      return false;
   }

   return true;
}

void
TitleListWorker::addEntriesToTitleList(const std::string &dirPath,
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
         emit entryReady(MakeTitleListEntry(path, iconPath, title, titleId, publisher, version));
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

   ForeachDirectoryEntry(dirPath, callback);
}

void
TitleListWorker::run()
{
   mStopProcessing = false;
   mWatchList.append(mDirPath);
   addEntriesToTitleList(mDirPath.toStdString(), mDeepScan ? 256 : 0);
   emit finished(mWatchList);
}

void
TitleListWorker::cancel()
{
   this->disconnect();
   mStopProcessing = true;
}
