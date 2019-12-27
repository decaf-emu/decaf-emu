#include "inputdriver.h"
#include "renderwidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWidget>
#include <QWindow>
#include <thread>

#include <libdecaf/decaf_graphics.h>
#include <libgpu/gpu_graphicsdriver.h>
#include <libgpu/gpu7_displaylayout.h>

#include <qpa/qplatformnativeinterface.h>

RenderWidget::RenderWidget(InputDriver *inputDriver, QWidget *parent) :
   mInputDriver(inputDriver),
   QWidget(parent)
{
   setAutoFillBackground(false);
   setAttribute(Qt::WA_PaintOnScreen);
   setAttribute(Qt::WA_OpaquePaintEvent);
   setAttribute(Qt::WA_NoSystemBackground);
   setAttribute(Qt::WA_NativeWindow);
   setFocusPolicy(Qt::StrongFocus);
   setFocus();
}

RenderWidget::~RenderWidget()
{
   if (mGraphicsDriver) {
      mRenderThread.join();
      delete mGraphicsDriver;
   }
}

void
RenderWidget::startGraphicsDriver()
{
   auto wsi = gpu::WindowSystemInfo { };
   auto platformName = QGuiApplication::platformName();
   if (platformName == QStringLiteral("windows")) {
      wsi.type = gpu::WindowSystemType::Windows;
   } else if (platformName == QStringLiteral("cocoa")) {
      wsi.type = gpu::WindowSystemType::Cocoa;
   } else if (platformName == QStringLiteral("xcb")) {
      wsi.type = gpu::WindowSystemType::X11;
   } else if (platformName == QStringLiteral("wayland")) {
      wsi.type = gpu::WindowSystemType::Wayland;
   }

   auto window = windowHandle();
#if defined(WIN32) || defined(__APPLE__)
   wsi.renderSurface = reinterpret_cast<void *>(window->winId());
#else
   auto pni = QGuiApplication::platformNativeInterface();
   wsi.displayConnection = pni->nativeResourceForWindow("display", window);
   if (wsi.type == gpu::WindowSystemType::Wayland) {
      wsi.renderSurface = pni->nativeResourceForWindow("surface", window);
   } else {
      wsi.renderSurface = reinterpret_cast<void *>(window->winId());
   }
#endif
   wsi.renderSurfaceScale = window->devicePixelRatio();

   mGraphicsDriver = gpu::createGraphicsDriver();
   mGraphicsDriver->setWindowSystemInfo(wsi);

   mRenderThread = std::thread { [this]() { mGraphicsDriver->run(); } };
   decaf::setGraphicsDriver(mGraphicsDriver);
}

QPaintEngine *
RenderWidget::paintEngine() const
{
   return nullptr;
}

bool
RenderWidget::event(QEvent *event)
{
   switch (event->type()) {
   case QEvent::Paint:
      return false;
   case QEvent::WinIdChange:
      if (mGraphicsDriver) {
         mGraphicsDriver->windowHandleChanged(reinterpret_cast<void *>(winId()));
      }
      break;
   case QEvent::KeyPress:
   {
      auto keyEvent = static_cast<QKeyEvent *>(event);
      mInputDriver->keyPressEvent(keyEvent->key());
      return true;
   }
   case QEvent::KeyRelease:
   {
      auto keyEvent = static_cast<QKeyEvent *>(event);
      mInputDriver->keyReleaseEvent(keyEvent->key());
      return true;
   }
   case QEvent::MouseMove:
   case QEvent::MouseButtonPress:
   case QEvent::MouseButtonRelease:
   {
      auto mouseEvent = static_cast<QMouseEvent *>(event);
      auto layout = gpu7::getDisplayLayout(static_cast<float>(width()),
                                           static_cast<float>(height()));
      auto touchEvent =
         gpu7::translateDisplayTouch(layout,
                                     static_cast<float>(mouseEvent->x()),
                                     static_cast<float>(mouseEvent->y()));

      if (touchEvent.screen != gpu7::DisplayTouchEvent::None) {
         mInputDriver->gamepadTouchEvent(
            !!(mouseEvent->buttons() & Qt::LeftButton),
            touchEvent.x, touchEvent.y);
      }
      return true;
   }
   case QEvent::Resize:
   {
      auto resizeEvent = static_cast<QResizeEvent *>(event);
      auto newSize = resizeEvent->size();
      auto desktop = QApplication::desktop();
      auto screenNumber = desktop->screenNumber(this);
      if (screenNumber == -1) {
         screenNumber = desktop->screenNumber(parentWidget());
      }

      auto devicePixelRatio = desktop->screen(screenNumber)->devicePixelRatio();
      if (mGraphicsDriver) {
         mGraphicsDriver->windowSizeChanged(newSize.width() * devicePixelRatio,
                                             newSize.height() * devicePixelRatio);
      }
      break;
   }
   }

   return QWidget::event(event);
}
