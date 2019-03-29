#pragma once
#include <QAbstractTableModel>

#include "debugdata.h"

class JitProfilingModel : public QAbstractTableModel
{
   Q_OBJECT

   static constexpr const char *ColumnNames[] = {
      "Address",
      "Native Code",
      "Time %",
      "Total Cycles",
      "Call Count",
      "Cycles/Call",
   };

   static constexpr int ColumnCount =
      static_cast<int>(sizeof(ColumnNames) / sizeof(ColumnNames[0]));

public:
   enum UserRole
   {
      SortRole = Qt::UserRole,
   };

   JitProfilingModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &JitProfilingModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->jitStats().compiledBlocks.size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return ColumnCount;
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant { };
      }

      const auto &jitStats = mDebugData->jitStats();
      if (index.row() >= jitStats.compiledBlocks.size() || index.row() < 0) {
         return QVariant { };
      }

      const auto totalTime = static_cast<double>(jitStats.totalTimeInCodeBlocks);
      const auto &stats = jitStats.compiledBlocks[index.row()];
      if (role == Qt::DisplayRole) {
         switch (index.column()) {
         case 0:
            return QString { "%1" }.arg(static_cast<uint>(stats.address), 8, 16, QChar { '0' });
         case 1:
            return QString { "%1" }.arg((quintptr)stats.code, QT_POINTER_SIZE * 2, 16, QChar{ '0' });
         case 2:
            if (totalTime == 0) {
               return QLatin1String { "0%" };
            } else {
               return QString{ "%1%" }.arg(100.0 * stats.profileData.time.load() / totalTime, 0, 'f', 2);
            }
         case 3:
            return QString { "%1" }.arg(stats.profileData.time.load());
         case 4:
            return QString { "%1" }.arg(stats.profileData.count.load());
         case 5:
         {
            auto count = stats.profileData.count.load();
            auto time = stats.profileData.time.load();
            return QString { "%1" }.arg(count ? (time + count / 2) / count : 0);
         }
         }
      } else if (role == SortRole) {
         switch (index.column()) {
         case 0:
            return stats.address;
         case 1:
            return reinterpret_cast<quintptr>(stats.code);
         case 2:
         case 3:
            return stats.profileData.time.load();
         case 4:
            return stats.profileData.count.load();
         case 5:
         {
            auto count = stats.profileData.count.load();
            auto time = stats.profileData.time.load();
            return count ? (time + count / 2) / count : 0ull;
         }
         }
      }

      return QVariant { };
   }

   QVariant headerData(int section, Qt::Orientation orientation, int role) const override
   {
      if (role != Qt::DisplayRole) {
         return QVariant { };
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
      auto newSize = static_cast<int>(mDebugData->jitStats().compiledBlocks.size());

      if (newSize < mPreviousSize) {
         beginRemoveRows({}, newSize, mPreviousSize - 1);
         endRemoveRows();
      } else if (newSize > mPreviousSize) {
         beginInsertRows({}, mPreviousSize, newSize - 1);
         endInsertRows();
      }

      dataChanged(index(0, 0), index(newSize, ColumnCount));
      mPreviousSize = newSize;
   }

private:
   DebugData *mDebugData = nullptr;
   int mPreviousSize = 0;
};
