#pragma once
#include "debugui.h"
#include "debugui_manager.h"

#ifdef DECAF_GL
#include <glbinding/gl/gl.h>

namespace debugui
{

class RendererOpenGL : public OpenGLRenderer
{
public:
   RendererOpenGL(const std::string &configPath);
   ~RendererOpenGL() override;

   void draw(unsigned width, unsigned height) override;

   bool
   onKeyAction(KeyboardKey key, KeyboardAction action) override
   {
      return mUi.onKeyAction(key, action);
   }

   bool
   onMouseAction(MouseButton button, MouseAction action) override
   {
      return mUi.onMouseAction(button, action);
   }

   bool
   onMouseMove(float x, float y) override
   {
      return mUi.onMouseMove(x, y);
   }

   bool
   onMouseScroll(float x, float y) override
   {
      return mUi.onMouseScroll(x, y);
   }

   bool
   onText(const char *text) override
   {
      return mUi.onText(text);
   }

   void
   setClipboardCallbacks(ClipboardTextGetCallback getClipboardFn,
                         ClipboardTextSetCallback setClipboardFn) override
   {
      return mUi.setClipboardCallbacks(getClipboardFn, setClipboardFn);
   }

private:
   Manager mUi;
   gl::GLuint mFontTexture;
   gl::GLuint mShaderHandle;
   gl::GLuint mVertHandle;
   gl::GLuint mFragHandle;
   gl::GLuint mVboHandle;
   gl::GLuint mVaoHandle;
   gl::GLuint mElementsHandle;
   gl::GLuint mAttribLocTex;
   gl::GLuint mAttribLocProjMtx;
   gl::GLuint mAttribLocPos;
   gl::GLuint mAttribLocUV;
   gl::GLuint mAttribLocColor;
};

} // namespace debugui

#endif // ifdef DECAF_GL
