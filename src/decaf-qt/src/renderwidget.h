#pragma once
#include <QWidget>
#include <thread>

namespace gpu
{
class GraphicsDriver;
}

class InputDriver;

class RenderWidget : public QWidget
{
public:
   RenderWidget(InputDriver *inputDriver, QWidget *parent = nullptr);
   ~RenderWidget();

   void startGraphicsDriver();

protected:
   QPaintEngine *paintEngine() const override;
   bool event(QEvent *event) override;

private:
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   InputDriver *mInputDriver = nullptr;
   std::thread mRenderThread;
};
