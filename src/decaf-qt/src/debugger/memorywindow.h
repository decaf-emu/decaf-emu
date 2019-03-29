#pragma once
#include <QWidget>
#include <memory>

namespace Ui
{
class MemoryWindow;
}

class MemoryWindow : public QWidget
{
   Q_OBJECT

public:
   MemoryWindow(QWidget *parent = nullptr);

   void navigateToAddress(uint32_t address);

protected slots:
   void addressChanged();
   void columnsChanged();

private:
   std::unique_ptr<Ui::MemoryWindow> ui;
};
