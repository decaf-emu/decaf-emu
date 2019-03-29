#pragma once
#include <QWidget>
#include <memory>

namespace Ui
{
class SegmentsWindow;
}

class DebugData;
class SegmentsModel;

class SegmentsWindow : public QWidget
{
   Q_OBJECT

public:
   explicit SegmentsWindow(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

signals:
   void navigateToDataAddress(uint32_t address);
   void navigateToTextAddress(uint32_t address);

protected slots:
   void segmentsViewDoubleClicked(const QModelIndex &index);

private:
   std::unique_ptr<Ui::SegmentsWindow> ui;

   DebugData *mDebugData = nullptr;
   SegmentsModel *mSegmentsModel = nullptr;
};
