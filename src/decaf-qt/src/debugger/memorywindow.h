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

   void navigateToAddress(uint32_t address);

protected slots:
   void addressChanged();
   void columnsChanged();

private:
   Ui::MemoryWindow *ui;
};
