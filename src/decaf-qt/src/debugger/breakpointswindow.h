#pragma once
#include <QWidget>

namespace Ui
{
class BreakpointsWindow;
}

class DebugData;
class BreakpointsModel;

class BreakpointsWindow : public QWidget
{
   Q_OBJECT

public:
   explicit BreakpointsWindow(QWidget *parent = nullptr);
   ~BreakpointsWindow();

   void setDebugData(DebugData *debugData);

signals:
   void navigateToTextAddress(uint32_t address);

protected slots:
   void breakpointsViewDoubleClicked(const QModelIndex &index);

protected:
   bool eventFilter(QObject *obj, QEvent *event) override;

private:
   Ui::BreakpointsWindow *ui;

   DebugData *mDebugData = nullptr;
   BreakpointsModel *mBreakpointsModel = nullptr;
};
