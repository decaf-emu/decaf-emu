#pragma once
#include <common/gl.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_opengl.h>
#include <SDL.h>
#include <string>

class SDLWindow
{
   static const auto WindowWidth = 1420;
   static const auto WindowHeight = 768;

public:
   ~SDLWindow();

   bool createWindow();
   bool run(const std::string &tracePath);

protected:
   void initialiseContext();
   void initialiseDraw();
   void drawScanBuffer(GLuint object);
   void calculateScreenViewports(float(&tv)[4],
                                 float(&drc)[4]);
   void drawScanBuffers(GLuint tvBuffer, GLuint drcBuffer);

private:
   SDL_Window *mWindow = nullptr;
   SDL_GLContext mWindowContext = nullptr;
   SDL_GLContext mGpuContext = nullptr;

   decaf::OpenGLDriver *mGraphicsDriver = nullptr;

   GLuint mVertexProgram;
   GLuint mPixelProgram;
   GLuint mPipeline;
   GLuint mVertArray;
   GLuint mVertBuffer;
   GLuint mSampler;
};
