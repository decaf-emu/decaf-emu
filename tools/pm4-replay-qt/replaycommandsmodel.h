#pragma once
#include <QAbstractTableModel>
#include "replay.h"

class ReplayCommandModel : public QAbstractTableModel
{
   Q_OBJECT
public:
   ReplayCommandModel(std::shared_ptr<ReplayFile> replay, QObject *parent = nullptr);

   int rowCount(const QModelIndex &parent = QModelIndex()) const;
   int columnCount(const QModelIndex &parent = QModelIndex()) const;
   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
   std::shared_ptr<ReplayFile> mReplay;
};
