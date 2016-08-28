#pragma once
#include <glbinding/gl/gl.h>
#include <libdecaf/decaf.h>
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
   void drawScanBuffer(gl::GLuint object);
   void calculateScreenViewports(float(&tv)[4],
                                 float(&drc)[4]);
   void drawScanBuffers(gl::GLuint tvBuffer, gl::GLuint drcBuffer);

private:
   SDL_Window *mWindow = nullptr;
   SDL_GLContext mWindowContext = nullptr;
   SDL_GLContext mGpuContext = nullptr;

   decaf::OpenGLDriver *mGraphicsDriver = nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
   gl::GLuint mSampler;
};
