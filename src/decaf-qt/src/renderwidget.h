#pragma once
#include <QWidget>
#include <thread>

namespace gpu
{
class GraphicsDriver;
}

class RenderWidget : public QWidget
{
public:
   RenderWidget(QWidget *parent = nullptr);
   ~RenderWidget();

   void startGraphicsDriver();

protected:
   QPaintEngine *paintEngine() const override;
   bool event(QEvent *event) override;

private:
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   std::thread mRenderThread;
};
