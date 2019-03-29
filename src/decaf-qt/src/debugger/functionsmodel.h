#pragma once
#include <QAbstractTableModel>

#include "debugdata.h"

class FunctionsModel : public QAbstractTableModel
{
   Q_OBJECT

   enum Columns
   {
      Name,
      Start,
      Length,
      Segment,
      NumColumns,
   };

   using AnalyseDatabase = DebugData::AnalyseDatabase;

public:
   enum UserRoles
   {
      StartAddressRole = Qt::UserRole,
   };

   FunctionsModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &FunctionsModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->analyseDatabase().functions.size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return NumColumns;
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant{ };
      }

      const auto &functions = mDebugData->analyseDatabase().functions;
      if (index.row() >= functions.size() || index.row() < 0) {
         return QVariant{ };
      }

      if (role == Qt::DisplayRole) {
         const auto &func = functions[index.row()];

         switch (index.column()) {
         case Columns::Name:
            return QString::fromStdString(func.name);
         case Columns::Start:
            return QString { "%1" }.arg(func.start, 8, 16, QChar { '0' }).toUpper();
         case Columns::Length:
            return QString { "%1" }.arg(func.end - func.start, 8, 16, QChar { '0' }).toUpper();
         case Columns::Segment:
         {
            auto segment = mDebugData->segmentForAddress(func.start);
            if (!segment) {
               return QString { "?" };
            }

            return QString::fromStdString(segment->name);
         }
         }
      } else if (role == StartAddressRole) {
         return functions[index.row()].start;
      }

      return QVariant { };
   }

   QVariant headerData(int section, Qt::Orientation orientation, int role) const override
   {
      if (role != Qt::DisplayRole) {
         return QVariant{ };
      }

      if (orientation == Qt::Horizontal) {
         switch (section) {
         case Columns::Name:
            return tr("Name");
         case Columns::Start:
            return tr("Start");
         case Columns::Length:
            return tr("Length");
         case Columns::Segment:
            return tr("Segment");
         }
      }

      return QVariant { };
   }

public slots:
   void debugDataChanged()
   {
      auto newSize = static_cast<int>(mDebugData->analyseDatabase().functions.size());

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
