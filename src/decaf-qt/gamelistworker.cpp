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

using DirectoryEntryCallable = std::function<bool(const QString &title,
                                                  const QString &titleId, 
                                                  const QString &path)>;

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
   str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

   return QString::fromWCharArray(str.c_str());
}

QList<QStandardItem*> 
MakeGameListEntry(const QString &path, 
                  const QString &title, 
                  const QString &titleId,
                  const std::vector<uint8_t> &icon) 
{
   // The game list uses this as compatibility number for untested games
   QList<QStandardItem*> list{
      new GameListItemPath(path, icon, title),
      new GameListItemCompat("99"), 
      new GameListItem(titleId.toStdString().c_str()),
      new GameListItem(path.toStdString().c_str()),
   };

   return list;
}
} // Anonymous namespace

GameListWorker::GameListWorker(QString dirPath, 
                               bool deepScan) : 
   mDirPath(std::move(dirPath)), mDeepScan(deepScan)
{
}

GameListWorker::~GameListWorker() = default;

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
   DIR* dirp = opendir(directory.c_str());
   if (!dirp)
     return false;

   // non windows loop
   while (struct dirent* result = readdir(dirp)) {
     const std::string virtual_name(result->d_name);
     QString fileName = QString::fromStdString(virtual_name);
#endif

     if (virtualName == "." || virtualName == "..") {
       continue;
     }

     // For some reason pugu doesn't like strings converted from QStrings
     std::string cosFileName = directory + "/" + virtualName + "/code/cos.xml";
     std::string metaFileName = directory + "/" + virtualName + "/meta/meta.xml";

     QString fullPath = QString::fromStdString(directory) + "/" + fileName;

     QString path = fullPath + "/code/" + getXmlElement(cosFileName, "app", "argstr");
     QString title = getXmlElement(metaFileName, "menu", "longname_en");
     QString titleId = getXmlElement(metaFileName, "menu", "title_id");

     if (!QFileInfo::exists(path)) {
        continue;
     }

     if (!callback(path, title, titleId)) {
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
GameListWorker::addEntriesToGameList(const std::string &dirPath, 
                                     unsigned int recursion) 
{
   const auto callback = [this, recursion](const QString &path,
                                           const QString &title,
                                           const QString &titleId) -> bool
   {
      if (mStopProcessing) {
         // Breaks the callback loop.
         return false;
      }

      std::vector<uint8_t> icon;
         
      emit entryReady(MakeGameListEntry(path,
                                        title,
                                        titleId,
                                        icon));

      return true;
   };

   ForeachDirectoryEntry(dirPath, callback);
}

void 
GameListWorker::run() 
{
   mStopProcessing = false;
   mWatchList.append(mDirPath);
   addEntriesToGameList(mDirPath.toStdString(), mDeepScan ? 256 : 0);
   emit finished(mWatchList);
}

void 
GameListWorker::cancel() 
{
   this->disconnect();
   mStopProcessing = true;
}
