#pragma once
#include <QMainWindow>
#include <DockManager.h>

#include <libdecaf/decaf_debug_api.h>

namespace Ui
{
class DebuggerWindow;
}

class DebugData;
class QTimer;

class DisassemblyWindow;
class FunctionsWindow;
class JitProfilingWindow;
class SegmentsWindow;
class ThreadsWindow;
class VoicesWindow;
class RegistersWindow;
class MemoryWindow;
class StackWindow;

class DebuggerWindow : public QMainWindow
{
   Q_OBJECT

public:
   explicit DebuggerWindow(QWidget *parent = 0);
   ~DebuggerWindow();

   void gotoTextAddress(decaf::debug::VirtualAddress address);
   void gotoDataAddress(decaf::debug::VirtualAddress address);

public slots:
   void updateModel();
   void onEntry();

   void debugPause();
   void debugResume();
   void debugStepOver();
   void debugStepInto();
   void debugToggleBreakpoint();

   void setHleTraceEnabled(bool enabled);

   void setGpuShaderBinaryDumpOnly(bool enabled);
   void setGpuShaderDumpEnabled(bool enabled);
   void setGx2ShaderDumpEnabled(bool enabled);
   void setGx2TextureDumpEnabled(bool enabled);

   void setPm4TraceEnabled(bool enabled);
   void pm4CaptureNextFrame();
   void pm4CaptureStateChanged(decaf::debug::Pm4CaptureState state);

   void activeThreadChanged();
   void executionStateChanged(bool paused);

   void focusChanged(QWidget *old, QWidget *now);

private:
   bool isDockWidgetFocused(QWidget *widget);

private:
   Ui::DebuggerWindow *ui;
   ads::CDockManager *mDockManager;

   DebugData *mDebugData;
   QTimer *mUpdateModelTimer;

   ads::CDockWidget *mDisassemblyDockWidget = nullptr;
   DisassemblyWindow *mDisassemblyWindow = nullptr;

   ads::CDockWidget *mFunctionsDockWidget = nullptr;
   FunctionsWindow *mFunctionsWindow = nullptr;

   ads::CDockWidget *mJitProfilingDockWidget = nullptr;
   JitProfilingWindow *mJitProfilingWindow = nullptr;

   ads::CDockWidget *mMemoryDockWidget = nullptr;
   MemoryWindow *mMemoryWindow = nullptr;

   ads::CDockWidget *mRegistersDockWidget = nullptr;
   RegistersWindow *mRegistersWindow = nullptr;

   ads::CDockWidget *mSegmentsDockWidget = nullptr;
   SegmentsWindow *mSegmentsWindow = nullptr;

   ads::CDockWidget *mStackDockWidget = nullptr;
   StackWindow *mStackWindow = nullptr;

   ads::CDockWidget *mThreadsDockWidget = nullptr;
   ThreadsWindow *mThreadsWindow = nullptr;

   ads::CDockWidget *mVoicesDockWidget = nullptr;
   VoicesWindow *mVoicesWindow = nullptr;
};
