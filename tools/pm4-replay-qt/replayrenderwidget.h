#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>

class ReplayRenderWidget : public QOpenGLWidget, QOpenGLFunctions_4_5_Core
{
   Q_OBJECT

public:
   ReplayRenderWidget(QWidget *parent = 0);

   void initializeGL() override;
   void paintGL() override;

public Q_SLOTS:
   void displayFrame(unsigned int tv, unsigned int drc);

private:
   GLuint mTvBuffer;
   GLuint mDrcBuffer;

   GLuint mVertexProgram;
   GLuint mPixelProgram;
   GLuint mPipeline;
   GLuint mVertArray;
   GLuint mVertBuffer;
   GLuint mSampler;
};
