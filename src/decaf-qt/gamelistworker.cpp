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

using DirectoryEntryCallable = std::function<bool(uint64_t* num_entries_out, 
                                                  const std::string& directory, 
                                                  const std::string& virtual_name)>;

namespace 
{

QString
getMetaValue(QString path, 
             const char *element, 
             const char *attribute)
{
   if (!platform::fileExists(path.toStdString())) {
     return "";
   }
   
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

   std::string str = child.text().get();
   str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

   return QString::fromStdString(str);
}

QString
getPath(QString path)
{
   path = path.replace("meta/meta.xml", "code/cos.xml");
   if (!platform::fileExists(path.toStdString())) {
     return "";
   }

   auto doc = pugi::xml_document{ };

   doc.load_file(path.toStdString().c_str());
   auto menu = doc.child("app");
   if (!menu) {
     return "";
   }

   auto child = menu.child("argstr");
   if (!child) {
     return "";
   }

   path = path.replace("cos.xml", child.text().get());
   std::string str = path.toStdString();
   return path;
}

QList<QStandardItem*> 
MakeGameListEntry(const std::string& path, 
                  const std::string& title, 
                  const std::string& titleId,
                  const std::vector<uint8_t>& icon) 
{
   // The game list uses this as compatibility number for untested games
   QList<QStandardItem*> list{
      new GameListItemPath(QString::fromStdString(path), icon,  QString::fromStdString(title)),
      new GameListItemCompat("99"), 
      new GameListItem(titleId.c_str()),
      new GameListItem(path.c_str()),
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
UTF8ToUTF16(const std::string& input) 
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
          const std::string& input) 
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
UTF8ToUTF16W(const std::string& input) 
{
   return CPToUTF16(CP_UTF8, input);
}

std::string
UTF16ToUTF8(const std::wstring& input) 
{
   const auto size = WideCharToMultiByte(CP_UTF8, 0, 
                                         input.data(), 
                                         static_cast<int>(input.size()),
                                         nullptr, 0, nullptr, nullptr);

   if (size == 0) {
     return "";
   }

   std::string output(size, '\0');

   if (size != WideCharToMultiByte(CP_UTF8, 
                                   0, 
                                   input.data(), 
                                   static_cast<int>(input.size()),
                                   &output[0], 
                                   static_cast<int>(output.size()), 
                                   nullptr,
                                   nullptr)) {
     output.clear();
   }

   return output;
}


bool 
ForeachDirectoryEntry(uint64_t* numEntriesOut, 
                      const std::string& directory,
                      DirectoryEntryCallable callback) 
{

   // How many files + directories we found
   uint64_t foundEntries = 0;

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
#else
   DIR* dirp = opendir(directory.c_str());
   if (!dirp)
     return false;

   // non windows loop
   while (struct dirent* result = readdir(dirp)) {
     const std::string virtual_name(result->d_name);
#endif

     if (virtualName == "." || virtualName == "..") {
       continue;
     }

     uint64_t retEntries = 0;
     if (!callback(&retEntries, directory, virtualName)) {
       callbackError = true;
       break;
     }
     foundEntries += retEntries;

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

   // num_entries_out is allowed to be specified nullptr, in which case we shouldn't try to set it
   if (numEntriesOut != nullptr) {
     *numEntriesOut = foundEntries;
   }
   return true;
}

void 
GameListWorker::addEntriesToGameList(const std::string& dirPath, 
                                     unsigned int recursion) 
{
   const auto callback = [this, recursion](uint64_t* numEntriesOut, 
                                           const std::string& directory,
                                           const std::string& virtualName) -> bool 
   {
      if (mStopProcessing) {
         // Breaks the callback loop.
         return false;
      }

      const std::string physicalName = directory + "/" + virtualName + "/meta/meta.xml";
      std::vector<uint8_t> icon;
         
      emit entryReady(MakeGameListEntry(getPath(physicalName.c_str()).toStdString(), 
                                        getMetaValue(physicalName.c_str(), "menu", "longname_en").toStdString(),
                                        getMetaValue(physicalName.c_str(), "menu", "title_id").toStdString(),
                                        icon));

      return true;
   };

   ForeachDirectoryEntry(nullptr, dirPath, callback);
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
