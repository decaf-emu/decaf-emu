#pragma once
#include <libdecaf/decaf_content.h>

#include <QAbstractTableModel>
#include <QDir>
#include <QString>
#include <QPixmap>

struct TitleInfo
{
   QString code_path;
   QString argstr;
   qulonglong title_id;
   int title_version;
   QString longname_en;
   QString publisher_en;
   QPixmap icon;
};

class TitleListModel : public QAbstractTableModel
{
   Q_OBJECT

public:
   static constexpr auto TitleTypeRole = Qt::UserRole;
   static constexpr auto TitlePathRole = Qt::UserRole + 1;
   static constexpr auto TitleIdRole = Qt::UserRole + 2;

   TitleListModel(QObject *parent = nullptr) :
      QAbstractTableModel(parent)
   {
   }

   ~TitleListModel()
   {
      qDeleteAll(mTitles);
   }

   void clear()
   {
      beginResetModel();
      qDeleteAll(mTitles);
      mTitles.clear();
      endResetModel();
   }

   int rowCount(const QModelIndex &parent) const override
   {
      if (parent.isValid()) {
         return 0;
      }

      return mTitles.size();
   }

   int columnCount(const QModelIndex &parent) const override
   {
      return 4;
   }

   static QString
   getTypeString(qulonglong titleId)
   {
      switch (decaf::getTitleTypeFromID(titleId)) {
      case decaf::TitleType::Application:
         return tr("Application");
      case decaf::TitleType::Demo:
         return tr("Demo");
      case decaf::TitleType::Data:
         return tr("Data");
      case decaf::TitleType::DLC:
         return tr("DLC");
      case decaf::TitleType::Update:
         return tr("Update");
      default:
         return tr("Unknown");
      }
   }

   QVariant data(const QModelIndex &index, int role) const override
   {
      if (!index.isValid()) {
         return QVariant { };
      }

      if (index.row() >= mTitles.size() || index.row() < 0) {
         return QVariant{ };
      }

      auto &title = *mTitles[index.row()];
      if (role == Qt::DisplayRole) {
         switch (index.column()) {
         case 0:
            return title.longname_en;
         case 1:
            return title.publisher_en;
         case 2:
            return getTypeString(title.title_id);
         case 3:
            return QDir { title.code_path }.filePath(title.argstr);
         default:
            return QVariant { };
         }
      } else if (role == Qt::DecorationRole) {
         if (index.column() == 0) {
            return title.icon;
         }
      } else if (role == TitleTypeRole) {
         return getTypeString(title.title_id);
      } else if (role == TitlePathRole) {
         return QDir { title.code_path }.filePath(title.argstr);
      } else if (role == TitleIdRole) {
         return title.title_id;
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
         case 0:
            return tr("Name");
         case 1:
            return tr("Publisher");
         case 2:
            return tr("Type");
         case 3:
            return tr("Path");
         }
      }

      return QVariant { };
   }

public slots:
   void addTitle(TitleInfo *info)
   {
      beginInsertRows({}, mTitles.size(), mTitles.size());
      mTitles.push_back(info);
      endInsertRows();
   }

private:
   QVector<TitleInfo *> mTitles;
};