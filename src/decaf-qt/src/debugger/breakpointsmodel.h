#pragma once
#include <QAbstractTableModel>
#include <vector>

#include "debugdata.h"

class BreakpointsModel : public QAbstractTableModel
{
   Q_OBJECT

   static constexpr const char *ColumnNames[] = {
      "Address",
      "Type",
   };

   static constexpr int ColumnCount =
      static_cast<int>(sizeof(ColumnNames) / sizeof(ColumnNames[0]));

   using CpuBreakpoint = decaf::debug::CpuBreakpoint;

public:
   enum UserRoles
   {
      AddressRole = Qt::UserRole,
   };

   BreakpointsModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &BreakpointsModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->breakpoints().size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return ColumnCount;
   }

   static QString getTypeString(CpuBreakpoint::Type type)
   {
      switch (type) {
      case CpuBreakpoint::Type::SingleFire:
         return tr("Single Fire");
      case CpuBreakpoint::Type::MultiFire:
         return tr("Multi Fire");
      default:
         return tr("Unknown");
      }
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant { };
      }

      const auto &breakpoints = mDebugData->breakpoints();
      if (index.row() >= breakpoints.size() || index.row() < 0) {
         return QVariant { };
      }

      if (role == Qt::DisplayRole) {
         const auto &breakpoint = breakpoints[index.row()];

         switch (index.column()) {
         case 0:
            return QString("%1").arg(static_cast<uint>(breakpoint.address), 8, 16, QChar { '0' });
         case 1:
            return getTypeString(breakpoint.type);
         }
      } else if (role == AddressRole) {
         return breakpoints[index.row()].address;
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
      auto newSize = static_cast<int>(mDebugData->breakpoints().size());

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
