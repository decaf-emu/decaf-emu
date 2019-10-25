#pragma once
#include <QWidget>

namespace Ui
{
class MemoryWindow;
}

class MemoryWindow : public QWidget
{
   Q_OBJECT

public:
   MemoryWindow(QWidget *parent = nullptr);
   ~MemoryWindow();

public slots:
   void navigateToAddress(uint32_t address);
   void navigateForward();
   void navigateBackward();

protected slots:
   void addressChanged();
   void columnsChanged();

private:
   Ui::MemoryWindow *ui;
};
