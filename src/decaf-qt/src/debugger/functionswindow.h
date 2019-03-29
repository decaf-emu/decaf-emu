#pragma once
#include <QWidget>
#include <memory>

namespace Ui
{
class FunctionsWindow;
}

class DebugData;
class FunctionsModel;
class QSortFilterProxyModel;

class FunctionsWindow : public QWidget
{
   Q_OBJECT

public:
   explicit FunctionsWindow(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

signals:
   void navigateToTextAddress(uint32_t address);

public slots:
   void filterChanged(QString value);
   void functionsViewDoubleClicked(const QModelIndex &index);

private:
   std::unique_ptr<Ui::FunctionsWindow> ui;

   DebugData *mDebugData = nullptr;
   FunctionsModel *mFunctionsModel = nullptr;
   QSortFilterProxyModel *mSortModel = nullptr;
};
