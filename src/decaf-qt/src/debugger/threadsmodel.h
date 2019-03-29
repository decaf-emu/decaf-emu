#pragma once
#include <QAbstractTableModel>

#include "debugdata.h"

class ThreadsModel : public QAbstractTableModel
{
   Q_OBJECT

   static constexpr const char *ColumnNames[] = {
      "ID",
      "Name",
      "NIA",
      "State",
      "Priority",
      "Affinity",
      "Core",
      "Core Time"
   };

   static constexpr int ColumnCount =
      static_cast<int>(sizeof(ColumnNames) / sizeof(ColumnNames[0]));

   using CafeThread = DebugData::CafeThread;

public:
   enum UserRoles
   {
      IdRole = Qt::UserRole,
   };

   ThreadsModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &ThreadsModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->threads().size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return ColumnCount;
   }

   static QString getThreadStateName(CafeThread::ThreadState state)
   {
      switch (state) {
      case CafeThread::Inactive:
         return tr("Inactive");
      case CafeThread::Ready:
         return tr("Ready");
      case CafeThread::Running:
         return tr("Running");
      case CafeThread::Waiting:
         return tr("Waiting");
      case CafeThread::Moribund:
         return tr("Moribund");
      default:
         return tr("Invalid");
      }
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant { };
      }

      auto &threads = mDebugData->threads();
      if (index.row() >= threads.size() || index.row() < 0) {
         return QVariant { };
      }

      const auto &threadInfo = threads[index.row()];
      if (role == Qt::DisplayRole) {
         switch (index.column()) {
         case 0:
            return QString("%1").arg(threadInfo.id);
         case 1:
            return QString::fromStdString(threadInfo.name);
         case 2:
            return QString("%1").arg(static_cast<uint>(threadInfo.nia), 8, 16, QChar{ '0' });
         case 3:
            return getThreadStateName(threadInfo.state);
         case 4:
            return QString("%1").arg(threadInfo.priority);
         case 5:
            return QString("%1").arg(threadInfo.affinity, 3, 2, QChar{ '0' });
         case 6:
            if (threadInfo.coreId == -1) {
               return QString();
            } else {
               return QString("%1").arg(threadInfo.coreId);
            }
         case 7:
            return QString("%1").arg(threadInfo.executionTime.count());
         }
      } else if (role == IdRole) {
         return threadInfo.id;
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
      auto newSize = static_cast<int>(mDebugData->threads().size());

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
