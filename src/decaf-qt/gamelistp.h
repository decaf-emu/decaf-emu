// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <fstream>
#include <unordered_map>
#include <utility>

#include <QCoreApplication>
#include <QImage>
#include <QObject>
#include <QStandardItem>
#include <QString>
#include <QWidget>

/**
 * Gets the default icon (for games without valid SMDH)
 * @param large If true, returns large icon (48x48), otherwise returns small icon (24x24)
 * @return QPixmap default icon
 */
static QPixmap getDefaultIcon(uint32_t size) 
{
   QPixmap icon(size, size);
   icon.fill(Qt::transparent);
   return icon;
}


class GameListItem : public QStandardItem 
{

public:
   GameListItem() = default;
   explicit GameListItem(const QString& string) : QStandardItem(string) {}
};


/**
 * A specialization of GameListItem for path values.
 * This class ensures that for every full path value it holds, a correct string representation
 * of just the filename (with no extension) will be displayed to the user.
 * If this class receives valid SMDH data, it will also display game icons and titles.
 */
class GameListItemPath : public GameListItem {
public:
   static const int FullPathRole = Qt::UserRole + 1;
   static const int TitleRole = Qt::UserRole + 2;
   static const int ProgramIdRole = Qt::UserRole + 3;
   static const int FileTypeRole = Qt::UserRole + 4;

   GameListItemPath() = default;
   GameListItemPath(const QString& gamePath, 
                    const std::vector<uint8_t>& pictureData,
                    const QString& gameName) 
   {
      setData(gamePath, FullPathRole);
      setData(gameName, TitleRole);
      
      const uint32_t size = 24;

      QPixmap picture;
      if (!picture.loadFromData(pictureData.data(), static_cast<uint32_t>(pictureData.size()))) {
         picture = getDefaultIcon(size);
      }
      picture = picture.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

      setData(picture, Qt::DecorationRole);      
   }

   
   QVariant data(int role) const override 
   {
      if (role == Qt::DisplayRole) {
         std::string filename;

         GameList::splitPath(data(FullPathRole).toString().toStdString(), 
                             nullptr, 
                             &filename,
                             nullptr);

         const std::array<QString, 3> rowData{{
            QString::fromStdString(filename),
            data(TitleRole).toString(),
         }};

         const auto& row1 = rowData.at(2);
         const auto& row2 = rowData.at(1);

         if (row1.isEmpty() || row1 == row2) {
            return row2;
         }
         if (row2.isEmpty()) {
            return row1;
         }

         return row1 + "\n   " + row2;
      }

      return GameListItem::data(role);
   }
};

class GameListItemCompat : public GameListItem 
{
   Q_DECLARE_TR_FUNCTIONS(GameListItemCompat)
public:
   static const int CompatNumberRole = Qt::UserRole + 1;
   GameListItemCompat() = default;

   explicit GameListItemCompat(const QString& compatibility) 
   {
      struct CompatStatus 
      {
         QString color;
         const char* text;
         const char* tooltip;
      };

      // clang-format off
      static const std::map<QString, CompatStatus> statusData = 
      {
         {"0",  {"#5c93ed", QT_TR_NOOP("Perfect"),   QT_TR_NOOP("Game functions flawless with no audio or graphical glitches, all tested functionality works as intended without\nany workarounds needed.")}},
         {"1",  {"#47d35c", QT_TR_NOOP("Great"),     QT_TR_NOOP("Game functions with minor graphical or audio glitches and is playable from start to finish. May require some\nworkarounds.")}},
         {"2",  {"#94b242", QT_TR_NOOP("Okay"),      QT_TR_NOOP("Game functions with major graphical or audio glitches, but game is playable from start to finish with\nworkarounds.")}},
         {"3",  {"#f2d624", QT_TR_NOOP("Bad"),      QT_TR_NOOP("Game functions, but with major graphical or audio glitches. Unable to progress in specific areas due to glitches\neven with workarounds.")}},
         {"4",  {"#FF0000", QT_TR_NOOP("Intro/Menu"), QT_TR_NOOP("Game is completely unplayable due to major graphical or audio glitches. Unable to progress past the Start\nScreen.")}},
         {"5",  {"#828282", QT_TR_NOOP("Won't Boot"), QT_TR_NOOP("The game crashes when attempting to start up.")}},
         {"99", {"#000000", QT_TR_NOOP("Not Tested"), QT_TR_NOOP("The game has not yet been tested.")}}
      };
      // clang-format on

      auto iterator = statusData.find(compatibility);
      if (iterator == statusData.end()) {
         return;
      }

      const CompatStatus& status = iterator->second;
      setData(compatibility, CompatNumberRole);
      setText(QObject::tr(status.text));
      setToolTip(QObject::tr(status.tooltip));
   }

   bool operator<(const QStandardItem& other) const override
   {
      return data(CompatNumberRole) < other.data(CompatNumberRole);
   }
};
