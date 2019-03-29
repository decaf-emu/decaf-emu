#pragma once
#include <QWidget>
#include <memory>

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

   void setDebugData(DebugData *debugData);

private:
   std::unique_ptr<Ui::VoicesWindow> ui;

   DebugData *mDebugData = nullptr;
   VoicesModel *mVoicesModel = nullptr;
};
