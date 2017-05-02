#pragma once
#include <libgpu/gpu_opengldriver.h>
#include <QThread>
#include <QObject>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QTimer>
#include <common/teenyheap.h>

class Decaf : public QObject
{
   Q_OBJECT

public:
   void start();
   void mainCoreEntry();

   QThread *thread()
   {
      return mThread;
   }

   QOpenGLContext *context()
   {
      return mContext;
   }

   QOffscreenSurface *surface()
   {
      return mSurface;
   }

   bool makeCurrent()
   {
      return mContext->makeCurrent(mSurface);
   }

   void doneCurrent()
   {
      mContext->doneCurrent();
   }

   void setContext(QOffscreenSurface *surface, QOpenGLContext *context)
   {
      mSurface = surface;
      mContext = context;
   }

   TeenyHeap *heap()
   {
      return mHeap;
   }

   gpu::OpenGLDriver *graphicsDriver()
   {
      return mGraphicsDriver;
   }

Q_SIGNALS:
   void started();

private:
   QThread *mThread = nullptr;
   TeenyHeap *mHeap = nullptr;
   QOffscreenSurface *mSurface = nullptr;
   QOpenGLContext *mContext = nullptr;
   gpu::OpenGLDriver *mGraphicsDriver;
};
