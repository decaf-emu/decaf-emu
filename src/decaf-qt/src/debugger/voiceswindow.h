#pragma once
#include <QWidget>

namespace Ui
{
class VoicesWindow;
}

class DebugData;
class VoicesModel;

class VoicesWindow : public QWidget
{
   Q_OBJECT

public:
   explicit VoicesWindow(QWidget *parent = nullptr);
   ~VoicesWindow();

   void setDebugData(DebugData *debugData);

private:
   Ui::VoicesWindow *ui;

   DebugData *mDebugData = nullptr;
   VoicesModel *mVoicesModel = nullptr;
};
