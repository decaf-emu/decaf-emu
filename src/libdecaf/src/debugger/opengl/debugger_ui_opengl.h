#pragma once

#ifdef DECAF_GL

#include "decaf_debugger.h"
#include <glbinding/gl/gl.h>

namespace debugger
{

namespace ui
{

class RendererOpenGL : public decaf::GLUiRenderer
{
public:
   virtual void initialise() override;
   virtual void postInitialize() override;
   virtual void draw(unsigned width, unsigned height) override;

private:
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

#endif // ifdef DECAF_GL
