#include "replaycommandsmodel.h"

ReplayCommandModel::ReplayCommandModel(std::shared_ptr<ReplayFile> replay, QObject *parent) :
   QAbstractTableModel(parent),
   mReplay(replay)
{
}

int ReplayCommandModel::rowCount(const QModelIndex &parent) const
{
   return static_cast<int>(mReplay->index.commands.size());
}

int ReplayCommandModel::columnCount(const QModelIndex &parent) const
{
   return 2;
}

QVariant ReplayCommandModel::data(const QModelIndex &index, int role) const
{
   if (role == Qt::DisplayRole) {
      if (index.column() == 0) {
         return QString { "%1" }.arg(index.row() + 1);
      } else if (index.column() == 1) {
         return QString::fromStdString(getCommandName(mReplay->index.commands[index.row()]));
      }
   }

   return QVariant {};
}
