#pragma once

#ifndef DECAF_NOGL

#include "decaf_debugger.h"
#include <glbinding/gl/gl.h>

namespace debugger
{

namespace ui
{

class RendererOpenGL : public decaf::DebugUiRenderer
{
public:
   virtual void initialise() override;
   virtual void draw(unsigned width, unsigned height)  override;

private:
   void loadFonts();

private:
   bool mFontsLoaded = false;
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

} // namespace ui

} // namespace debugger

#endif // DECAF_NOGL
