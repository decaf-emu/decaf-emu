#pragma once
#include <QWidget>

namespace Ui
{
class JitProfilingWindow;
}

class DebugData;
class JitProfilingModel;
class QSortFilterProxyModel;

class JitProfilingWindow : public QWidget
{
   Q_OBJECT

public:
   JitProfilingWindow(QWidget *parent = nullptr);
   ~JitProfilingWindow();

   void setDebugData(DebugData *debugData);

signals:
   void navigateToTextAddress(uint32_t address);

public slots:
   void clearProfileData();
   void setProfilingEnabled(bool enabled);
   void setCore0Mask(bool enabled);
   void setCore1Mask(bool enabled);
   void setCore2Mask(bool enabled);
   void tableViewDoubleClicked(QModelIndex index);

private:
   Ui::JitProfilingWindow *ui;
   DebugData *mDebugData = nullptr;
   JitProfilingModel *mJitProfilingModel = nullptr;
   QSortFilterProxyModel *mSortModel = nullptr;
};
