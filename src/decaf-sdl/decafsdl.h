#pragma once
#include "libdecaf/decaf.h"
#include <SDL.h>
#include <glbinding/gl/gl.h>

using namespace decaf::input;

class DecafSDL : public decaf::InputDriver
{
   static const auto WindowWidth = 1420;
   static const auto WindowHeight = 768;

public:
   ~DecafSDL();

   bool createWindow();
   bool run(const std::string &gamePath);

private:
   void initialiseContext();
   void initialiseDraw();
   void drawScanBuffer(gl::GLuint object);
   void drawScanBuffers(gl::GLuint tvBuffer, gl::GLuint drcBuffer);

   decaf::input::KeyboardKey
   translateKeyCode(SDL_Keysym sym);

   decaf::input::MouseButton
   translateMouseButton(int button);

   // VPAD
   virtual vpad::Type
   getControllerType(vpad::Channel channel) override;

   virtual ButtonStatus
   getButtonStatus(vpad::Channel channel, vpad::Core button) override;

   virtual float
   getAxisValue(vpad::Channel channel, vpad::CoreAxis axis) override;

   // WPAD
   virtual wpad::Type
   getControllerType(wpad::Channel channel) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Core button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Classic button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Nunchuck button) override;

   virtual ButtonStatus
   getButtonStatus(wpad::Channel channel, wpad::Pro button) override;

   virtual float
   getAxisValue(wpad::Channel channel, wpad::NunchuckAxis axis) override;

   virtual float
   getAxisValue(wpad::Channel channel, wpad::ProAxis axis) override;

private:
   std::thread mGraphicsThread;
   decaf::OpenGLDriver *mGraphicsDriver = nullptr;

   SDL_Window *mWindow = nullptr;
   SDL_GLContext mContext = nullptr;
   SDL_GLContext mThreadContext = nullptr;

   gl::GLuint mVertexProgram;
   gl::GLuint mPixelProgram;
   gl::GLuint mPipeline;
   gl::GLuint mVertArray;
   gl::GLuint mVertBuffer;
};
