#ifdef DECAF_GL
#include "debugui.h"
#include "debugui_manager.h"
#include "debugui_renderer_opengl.h"

#include <imgui.h>
#include <gsl.h>
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>

namespace debugui
{

static const gl::GLchar *sVertexShader =
   "#version 330\n"
   "uniform mat4 ProjMtx;\n"
   "in vec2 Position;\n"
   "in vec2 UV;\n"
   "in vec4 Color;\n"
   "out vec2 Frag_UV;\n"
   "out vec4 Frag_Color;\n"
   "void main()\n"
   "{\n"
   "	Frag_UV = UV;\n"
   "	Frag_Color = Color;\n"
   "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
   "}\n";

static const gl::GLchar *sFragmentShader =
   "#version 330\n"
   "uniform sampler2D Texture;\n"
   "in vec2 Frag_UV;\n"
   "in vec4 Frag_Color;\n"
   "out vec4 Out_Color;\n"
   "void main()\n"
   "{\n"
   "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
   "}\n";

RendererOpenGL::RendererOpenGL(const std::string &configPath) :
   mUi(configPath)
{
   // Initialise OpenGL
   mShaderHandle = gl::glCreateProgram();
   mVertHandle = gl::glCreateShader(gl::GL_VERTEX_SHADER);
   mFragHandle = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);
   gl::glShaderSource(mVertHandle, 1, &sVertexShader, 0);
   gl::glShaderSource(mFragHandle, 1, &sFragmentShader, 0);
   gl::glCompileShader(mVertHandle);
   gl::glCompileShader(mFragHandle);
   gl::glAttachShader(mShaderHandle, mVertHandle);
   gl::glAttachShader(mShaderHandle, mFragHandle);
   gl::glLinkProgram(mShaderHandle);

   mAttribLocTex = gl::glGetUniformLocation(mShaderHandle, "Texture");
   mAttribLocProjMtx = gl::glGetUniformLocation(mShaderHandle, "ProjMtx");
   mAttribLocPos = gl::glGetAttribLocation(mShaderHandle, "Position");
   mAttribLocUV = gl::glGetAttribLocation(mShaderHandle, "UV");
   mAttribLocColor = gl::glGetAttribLocation(mShaderHandle, "Color");

   gl::glGenBuffers(1, &mVboHandle);
   gl::glGenBuffers(1, &mElementsHandle);

   gl::glGenVertexArrays(1, &mVaoHandle);
   gl::glBindVertexArray(mVaoHandle);
   gl::glBindBuffer(gl::GL_ARRAY_BUFFER, mVboHandle);
   gl::glEnableVertexAttribArray(mAttribLocPos);
   gl::glEnableVertexAttribArray(mAttribLocUV);
   gl::glEnableVertexAttribArray(mAttribLocColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
   gl::glVertexAttribPointer(mAttribLocPos, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, pos));
   gl::glVertexAttribPointer(mAttribLocUV, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, uv));
   gl::glVertexAttribPointer(mAttribLocColor, 4, gl::GL_UNSIGNED_BYTE, gl::GL_TRUE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

   // Load imgui fonts
   auto &io = ImGui::GetIO();
   unsigned char *pixels;
   int width, height;
   io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

   gl::glGenTextures(1, &mFontTexture);
   gl::glBindTexture(gl::GL_TEXTURE_2D, mFontTexture);
   gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, static_cast<gl::GLint>(gl::GL_LINEAR));
   gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, static_cast<gl::GLint>(gl::GL_LINEAR));
   gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, static_cast<gl::GLint>(gl::GL_RGBA), width, height, 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, pixels);
   io.Fonts->TexID = reinterpret_cast<void*>(static_cast<int64_t>(mFontTexture));
}

RendererOpenGL::~RendererOpenGL()
{
   // TODO: Cleanup our resources
}

struct State
{
   gl::GLint last_program;
   gl::GLint last_texture;
   gl::GLint last_active_texture;
   gl::GLint last_array_buffer;
   gl::GLint last_element_array_buffer;
   gl::GLint last_vertex_array;
   gl::GLint last_blend_src;
   gl::GLint last_blend_dst;
   gl::GLint last_blend_equation_rgb;
   gl::GLint last_blend_equation_alpha;
   gl::GLint last_viewport[4];
   gl::GLint last_scissor[4];
   gl::GLboolean last_enable_blend;
   gl::GLboolean last_enable_cull_face;
   gl::GLboolean last_enable_depth_test;
   gl::GLboolean last_enable_scissor_test;
};

static void
saveState(State &state)
{
   gl::glGetIntegerv(gl::GL_CURRENT_PROGRAM, &state.last_program);
   gl::glGetIntegerv(gl::GL_TEXTURE_BINDING_2D, &state.last_texture);
   gl::glGetIntegerv(gl::GL_ACTIVE_TEXTURE, &state.last_active_texture);
   gl::glGetIntegerv(gl::GL_ARRAY_BUFFER_BINDING, &state.last_array_buffer);
   gl::glGetIntegerv(gl::GL_ELEMENT_ARRAY_BUFFER_BINDING, &state.last_element_array_buffer);
   gl::glGetIntegerv(gl::GL_VERTEX_ARRAY_BINDING, &state.last_vertex_array);
   gl::glGetIntegerv(gl::GL_BLEND_SRC, &state.last_blend_src);
   gl::glGetIntegerv(gl::GL_BLEND_DST, &state.last_blend_dst);
   gl::glGetIntegerv(gl::GL_BLEND_EQUATION_RGB, &state.last_blend_equation_rgb);
   gl::glGetIntegerv(gl::GL_BLEND_EQUATION_ALPHA, &state.last_blend_equation_alpha);
   gl::glGetIntegerv(gl::GL_VIEWPORT, state.last_viewport);
   gl::glGetIntegerv(gl::GL_SCISSOR_BOX, state.last_scissor);
   state.last_enable_blend = gl::glIsEnabled(gl::GL_BLEND);
   state.last_enable_cull_face = gl::glIsEnabled(gl::GL_CULL_FACE);
   state.last_enable_depth_test = gl::glIsEnabled(gl::GL_DEPTH_TEST);
   state.last_enable_scissor_test = gl::glIsEnabled(gl::GL_SCISSOR_TEST);
}

static void
loadState(State &state)
{
   gl::glUseProgram(state.last_program);
   gl::glActiveTexture(static_cast<gl::GLenum>(state.last_active_texture));
   gl::glBindTexture(gl::GL_TEXTURE_2D, state.last_texture);
   gl::glBindVertexArray(state.last_vertex_array);
   gl::glBindBuffer(gl::GL_ARRAY_BUFFER, state.last_array_buffer);
   gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, state.last_element_array_buffer);

   gl::glBlendEquationSeparate(static_cast<gl::GLenum>(state.last_blend_equation_rgb),
                               static_cast<gl::GLenum>(state.last_blend_equation_alpha));

   gl::glBlendFunc(static_cast<gl::GLenum>(state.last_blend_src),
                   static_cast<gl::GLenum>(state.last_blend_dst));

   if (state.last_enable_blend == gl::GL_TRUE) {
      gl::glEnable(gl::GL_BLEND);
   } else {
      gl::glDisable(gl::GL_BLEND);
   }

   if (state.last_enable_cull_face == gl::GL_TRUE) {
      gl::glEnable(gl::GL_CULL_FACE);
   } else {
      gl::glDisable(gl::GL_CULL_FACE);
   }

   if (state.last_enable_depth_test == gl::GL_TRUE) {
      gl::glEnable(gl::GL_DEPTH_TEST);
   } else {
      gl::glDisable(gl::GL_DEPTH_TEST);
   }

   if (state.last_enable_scissor_test == gl::GL_TRUE) {
      gl::glEnable(gl::GL_SCISSOR_TEST);
   } else {
      gl::glDisable(gl::GL_SCISSOR_TEST);
   }

   gl::glViewport(state.last_viewport[0],
                  state.last_viewport[1],
                  static_cast<gl::GLsizei>(state.last_viewport[2]),
                  static_cast<gl::GLsizei>(state.last_viewport[3]));

   gl::glScissor(state.last_scissor[0],
                 state.last_scissor[1],
                 state.last_scissor[2],
                 state.last_scissor[3]);
}

void RendererOpenGL::draw(unsigned width, unsigned height)
{
   // Draw the ui
   mUi.draw(width, height);

   // Scale clip rects
   auto &io = ImGui::GetIO();
   auto drawData = ImGui::GetDrawData();
   auto fbWidth = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
   auto fbHeight = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
   drawData->ScaleClipRects(io.DisplayFramebufferScale);

   // Backup GL state
   auto state = State {};
   saveState(state);

   // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
   gl::glEnable(gl::GL_BLEND);
   gl::glBlendEquation(gl::GL_FUNC_ADD);
   gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
   gl::glDisable(gl::GL_CULL_FACE);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glEnable(gl::GL_SCISSOR_TEST);
   gl::glActiveTexture(gl::GL_TEXTURE0);

   // Setup viewport, orthographic projection matrix
   gl::glViewport(0, 0, static_cast<gl::GLsizei>(fbWidth), (gl::GLsizei)fbHeight);
   const float ortho_projection[4][4] =
   {
      {  2.0f / io.DisplaySize.x, 0.0f,                      0.0f, 0.0f },
      {  0.0f,                    2.0f / -io.DisplaySize.y,  0.0f, 0.0f },
      {  0.0f,                    0.0f,                     -1.0f, 0.0f },
      { -1.0f,                    1.0f,                      0.0f, 1.0f },
   };
   gl::glUseProgram(mShaderHandle);
   gl::glUniform1i(mAttribLocTex, 0);
   gl::glUniformMatrix4fv(mAttribLocProjMtx, 1, gl::GL_FALSE, &ortho_projection[0][0]);
   gl::glBindVertexArray(mVaoHandle);

   for (auto n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList *cmdList = drawData->CmdLists[n];
      const ImDrawIdx *idxBufOffset = nullptr;

      gl::glBindBuffer(gl::GL_ARRAY_BUFFER, mVboHandle);
      gl::glBufferData(gl::GL_ARRAY_BUFFER,
                       static_cast<gl::GLsizeiptr>(cmdList->VtxBuffer.size() * sizeof(ImDrawVert)),
                       reinterpret_cast<const gl::GLvoid *>(&cmdList->VtxBuffer.front()),
                       gl::GL_STREAM_DRAW);

      gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, mElementsHandle);
      gl::glBufferData(gl::GL_ELEMENT_ARRAY_BUFFER, (gl::GLsizeiptr)cmdList->IdxBuffer.size() * sizeof(ImDrawIdx), (gl::GLvoid*)&cmdList->IdxBuffer.front(), gl::GL_STREAM_DRAW);

      for (auto pcmd = cmdList->CmdBuffer.begin(); pcmd != cmdList->CmdBuffer.end(); pcmd++) {
         if (pcmd->UserCallback) {
            pcmd->UserCallback(cmdList, pcmd);
         } else {
            gl::glBindTexture(gl::GL_TEXTURE_2D, (gl::GLuint)(intptr_t)pcmd->TextureId);
            gl::glScissor((int)pcmd->ClipRect.x, (int)(fbHeight - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
            gl::glDrawElements(gl::GL_TRIANGLES, (gl::GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? gl::GL_UNSIGNED_SHORT : gl::GL_UNSIGNED_INT, idxBufOffset);
         }

         idxBufOffset += pcmd->ElemCount;
      }
   }

   // Restore modified GL state
   loadState(state);
}

} // namespace debugui

#endif // ifdef DECAF_GL
