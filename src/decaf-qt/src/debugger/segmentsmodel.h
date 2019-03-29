#pragma once
#include <QAbstractTableModel>
#include <vector>

#include "debugdata.h"

class SegmentsModel : public QAbstractTableModel
{
   Q_OBJECT

   static constexpr const char *ColumnNames[] = {
      "Name",
      "Start",
      "End",
      "R",
      "W",
      "X",
      "Align",
   };

   static constexpr int ColumnCount =
      static_cast<int>(sizeof(ColumnNames) / sizeof(ColumnNames[0]));

   using CafeMemorySegment = DebugData::CafeMemorySegment;

public:
   enum UserRoles
   {
      AddressRole = Qt::UserRole,
      ExecuteRole,
   };

   SegmentsModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &SegmentsModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->segments().size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return ColumnCount;
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant{ };
      }

      const auto &segments = mDebugData->segments();
      if (index.row() >= segments.size() || index.row() < 0) {
         return QVariant { };
      }

      if (role == Qt::DisplayRole) {
         const auto &segment = segments[index.row()];

         switch (index.column()) {
         case 0:
            return QString::fromStdString(segment.name);
         case 1:
            return QString("%1").arg(static_cast<uint>(segment.address), 8, 16, QChar { '0' });
         case 2:
            return QString("%1").arg(static_cast<uint>(segment.address + segment.size), 8, 16, QChar { '0' });
         case 3:
            return segment.read ? "R" : ".";
         case 4:
            return segment.write ? "W" : ".";
         case 5:
            return segment.execute ? "X" : ".";
         case 6:
            return QString("0x%1").arg(static_cast<uint>(segment.align), 0, 16, QChar { '0' });
         }
      } else if (role == AddressRole) {
         return segments[index.row()].address;
      } else if (role == ExecuteRole) {
         return segments[index.row()].execute;
      }

      return QVariant { };
   }

   QVariant headerData(int section, Qt::Orientation orientation, int role) const override
   {
      if (role != Qt::DisplayRole) {
         return QVariant{ };
      }

      if (orientation == Qt::Horizontal) {
         if (section < ColumnCount) {
            return ColumnNames[section];
         }
      }

      return QVariant { };
   }

private slots:
   void debugDataChanged()
   {
      auto newSize = static_cast<int>(mDebugData->segments().size());

      if (newSize < mPreviousSize) {
         beginRemoveRows({}, newSize, mPreviousSize - 1);
         endRemoveRows();
      } else if (newSize > mPreviousSize) {
         beginInsertRows({}, mPreviousSize, newSize - 1);
         endInsertRows();
      }

      mPreviousSize = newSize;
   }

private:
   DebugData *mDebugData = nullptr;
   int mPreviousSize = 0;
};
