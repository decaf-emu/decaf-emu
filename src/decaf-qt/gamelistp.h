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

#include "tga.h"

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

class TitleListItem : public QStandardItem 
{
public:
   TitleListItem() = default;
   explicit TitleListItem(const QString &string) : QStandardItem(string) {}
};


/**
 * A specialization of GameListItem for path values.
 * This class ensures that for every full path value it holds, a correct string representation
 * of just the filename (with no extension) will be displayed to the user.
 * If this class receives valid SMDH data, it will also display game icons and titles.
 */
class TitleListItemPath : public TitleListItem {
public:
   static const int FullPathRole = Qt::UserRole + 1;
   static const int TitleRole = Qt::UserRole + 2;
   static const int TitleIdRole = Qt::UserRole + 3;
   static const int FileTypeRole = Qt::UserRole + 4;

   TitleListItemPath() = default;

   TitleListItemPath(const QString &titlePath,
                     const QString &iconPath,
                     const QString &titleName,
                     const QString &titleId)
   {
      setData(titlePath, FullPathRole);
      setData(titleName, TitleRole);
      setData(titleId, TitleIdRole);

      uint32_t size = 64;

      QFile file(iconPath);

      if (!file.open(QIODevice::ReadOnly)) {
         QPixmap picture = getDefaultIcon(size);
         setData(picture, Qt::DecorationRole);
      } else {
         QImage image;
         TGAHandler::read(QDataStream(&file), &image);
         setData(image.scaled(size, size, Qt::KeepAspectRatio), Qt::DecorationRole);
      }
   }

   QVariant data(int role) const override 
   {
      if (role == Qt::DisplayRole) {
         const std::array<QString, 2> rowData{{
            data(TitleRole).toString(),
            data(TitleIdRole).toString()
         }};

         const auto &row1 = rowData.at(0);
         const auto &row2 = rowData.at(1);

         if (row1.isEmpty() || row1 == row2) {
            return row2;
         }
         if (row2.isEmpty()) {
            return row1;
         }

         return row1 + "\n    [" + row2 + "]";
      }

      return TitleListItem::data(role);
   }
};
