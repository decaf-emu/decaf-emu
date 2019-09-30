#pragma once
#include <QWidget>

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
   ~FunctionsWindow();

   void setDebugData(DebugData *debugData);

signals:
   void navigateToTextAddress(uint32_t address);

public slots:
   void filterChanged(QString value);
   void functionsViewDoubleClicked(const QModelIndex &index);

private:
   Ui::FunctionsWindow *ui;

   DebugData *mDebugData = nullptr;
   FunctionsModel *mFunctionsModel = nullptr;
   QSortFilterProxyModel *mSortModel = nullptr;
};
