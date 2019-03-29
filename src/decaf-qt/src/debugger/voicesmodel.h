#pragma once
#include <QAbstractTableModel>
#include <vector>

#include "debugdata.h"

class VoicesModel : public QAbstractTableModel
{
   Q_OBJECT

   static constexpr const char *ColumnNames[] = {
      "ID",
      "State",
      "Format",
      "Stream",
      "Address",
      "Current Offset",
      "End Offset",
      "Loop Offset",
      "Loop Mode",
   };

   static constexpr int ColumnCount =
      static_cast<int>(sizeof(ColumnNames) / sizeof(ColumnNames[0]));

   using CafeVoice = DebugData::CafeVoice;

public:
   VoicesModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   void setDebugData(DebugData *debugData)
   {
      mDebugData = debugData;
      connect(mDebugData, &DebugData::dataChanged, this, &VoicesModel::debugDataChanged);
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (!mDebugData) {
         return 0;
      }

      return static_cast<int>(mDebugData->voices().size());
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return ColumnCount;
   }

   static QString getStateString(CafeVoice::State state)
   {
      switch (state) {
      case CafeVoice::State::Stopped:
         return tr("Stopped");
      case CafeVoice::State::Playing:
         return tr("Playing");
      default:
         return tr("Unknown");
      }
   }

   static QString getFormatString(CafeVoice::Format format)
   {
      switch (format) {
      case CafeVoice::Format::ADPCM:
         return tr("ADPCM");
      case CafeVoice::Format::LPCM16:
         return tr("LPCM16");
      case CafeVoice::Format::LPCM8:
         return tr("LPCM8");
      default:
         return tr("Unknown");
      }
   }

   static QString getVoiceTypeString(CafeVoice::VoiceType type)
   {
      switch (type) {
      case CafeVoice::VoiceType::Default:
         return tr("Default");
      case CafeVoice::VoiceType::Streaming:
         return tr("Streaming");
      default:
         return tr("Unknown");
      }
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!mDebugData || !index.isValid()) {
         return QVariant{ };
      }

      const auto &voices = mDebugData->voices();
      if (index.row() >= voices.size() || index.row() < 0) {
         return QVariant{ };
      }

      if (role == Qt::DisplayRole) {
         const auto &voice = voices.at(index.row());

         switch (index.column()) {
         case 0:
            return QString("%1").arg(voice.index);
         case 1:
            return getStateString(voice.state);
         case 2:
            return getFormatString(voice.format);
         case 3:
            return getVoiceTypeString(voice.type);
         case 4:
            return QString("%1").arg(static_cast<uint>(voice.data), 8, 16, QChar{ '0' });
         case 5:
            return QString("%1").arg(voice.currentOffset);
         case 6:
            return QString("%1").arg(voice.endOffset);
         case 7:
            return QString("%1").arg(voice.loopOffset);
         case 8:
            return voice.loopingEnabled ? tr("Looping") : QString("");
         }
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

private:
   void debugDataChanged()
   {
      auto newSize = static_cast<int>(mDebugData->voices().size());

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
