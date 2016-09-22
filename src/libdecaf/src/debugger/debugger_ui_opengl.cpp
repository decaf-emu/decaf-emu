#ifndef DECAF_NOGL

#include "debugger/debugger_ui.h"
#include "decaf.h"
#include "decaf_debugger.h"
#include <imgui.h>
#include <gsl.h>
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>

namespace decaf
{

namespace debugger
{

struct DebugDrawData
{
   gl::GLuint fontTexture;
   gl::GLuint shaderHandle;
   gl::GLuint vertHandle;
   gl::GLuint fragHandle;
   gl::GLuint vboHandle;
   gl::GLuint vaoHandle;
   gl::GLuint elementsHandle;
   gl::GLuint attribLocTex;
   gl::GLuint attribLocProjMtx;
   gl::GLuint attribLocPos;
   gl::GLuint attribLocUV;
   gl::GLuint attribLocColor;
};

static DebugDrawData
sDebugDrawData;

void
initialiseUiGL()
{
   auto &data = sDebugDrawData;
   ImGuiIO& io = ImGui::GetIO();

   auto configPath = makeConfigPath("imgui.ini");
   ::debugger::ui::initialise(configPath);

   const gl::GLchar *vertex_shader =
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

   const gl::GLchar* fragment_shader =
      "#version 330\n"
      "uniform sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Color;\n"
      "out vec4 Out_Color;\n"
      "void main()\n"
      "{\n"
      "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
      "}\n";

   data.shaderHandle = gl::glCreateProgram();
   data.vertHandle = gl::glCreateShader(gl::GL_VERTEX_SHADER);
   data.fragHandle = gl::glCreateShader(gl::GL_FRAGMENT_SHADER);
   gl::glShaderSource(data.vertHandle, 1, &vertex_shader, 0);
   gl::glShaderSource(data.fragHandle, 1, &fragment_shader, 0);
   gl::glCompileShader(data.vertHandle);
   gl::glCompileShader(data.fragHandle);
   gl::glAttachShader(data.shaderHandle, data.vertHandle);
   gl::glAttachShader(data.shaderHandle, data.fragHandle);
   gl::glLinkProgram(data.shaderHandle);

   data.attribLocTex = gl::glGetUniformLocation(data.shaderHandle, "Texture");
   data.attribLocProjMtx = gl::glGetUniformLocation(data.shaderHandle, "ProjMtx");
   data.attribLocPos = gl::glGetAttribLocation(data.shaderHandle, "Position");
   data.attribLocUV = gl::glGetAttribLocation(data.shaderHandle, "UV");
   data.attribLocColor = gl::glGetAttribLocation(data.shaderHandle, "Color");

   gl::glGenBuffers(1, &data.vboHandle);
   gl::glGenBuffers(1, &data.elementsHandle);

   gl::glGenVertexArrays(1, &data.vaoHandle);
   gl::glBindVertexArray(data.vaoHandle);
   gl::glBindBuffer(gl::GL_ARRAY_BUFFER, data.vboHandle);
   gl::glEnableVertexAttribArray(data.attribLocPos);
   gl::glEnableVertexAttribArray(data.attribLocUV);
   gl::glEnableVertexAttribArray(data.attribLocColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
   gl::glVertexAttribPointer(data.attribLocPos, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, pos));
   gl::glVertexAttribPointer(data.attribLocUV, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, uv));
   gl::glVertexAttribPointer(data.attribLocColor, 4, gl::GL_UNSIGNED_BYTE, gl::GL_TRUE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

   // Upload the font texture
   unsigned char* pixels;
   int width, height;
   io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

   gl::glGenTextures(1, &data.fontTexture);
   gl::glBindTexture(gl::GL_TEXTURE_2D, data.fontTexture);
   gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, static_cast<gl::GLint>(gl::GL_LINEAR));
   gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, static_cast<gl::GLint>(gl::GL_LINEAR));
   gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, static_cast<gl::GLint>(gl::GL_RGBA), width, height, 0, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, pixels);

   io.Fonts->TexID = reinterpret_cast<void*>(static_cast<int64_t>(data.fontTexture));
}

void
drawUiGL(uint32_t width, uint32_t height)
{
   ImGuiIO& io = ImGui::GetIO();

   // Update some per-frame state information
   io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
   io.DeltaTime = 1.0f / 60.0f;
   ::debugger::ui::updateInput();

   // Start the frame
   ImGui::NewFrame();

   ::debugger::ui::draw();

   ImGui::Render();
   auto &data = sDebugDrawData;
   auto drawData = ImGui::GetDrawData();
   int fbWidth = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
   int fbHeight = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
   drawData->ScaleClipRects(io.DisplayFramebufferScale);

   // Backup GL state
   gl::GLint last_program; gl::glGetIntegerv(gl::GL_CURRENT_PROGRAM, &last_program);
   gl::GLint last_texture; gl::glGetIntegerv(gl::GL_TEXTURE_BINDING_2D, &last_texture);
   gl::GLint last_active_texture; gl::glGetIntegerv(gl::GL_ACTIVE_TEXTURE, &last_active_texture);
   gl::GLint last_array_buffer; gl::glGetIntegerv(gl::GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
   gl::GLint last_element_array_buffer; gl::glGetIntegerv(gl::GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
   gl::GLint last_vertex_array; gl::glGetIntegerv(gl::GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
   gl::GLint last_blend_src; gl::glGetIntegerv(gl::GL_BLEND_SRC, &last_blend_src);
   gl::GLint last_blend_dst; gl::glGetIntegerv(gl::GL_BLEND_DST, &last_blend_dst);
   gl::GLint last_blend_equation_rgb; gl::glGetIntegerv(gl::GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
   gl::GLint last_blend_equation_alpha; gl::glGetIntegerv(gl::GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
   gl::GLint last_viewport[4]; gl::glGetIntegerv(gl::GL_VIEWPORT, last_viewport);
   gl::GLint last_scissor[4]; gl::glGetIntegerv(gl::GL_SCISSOR_BOX, last_scissor);
   gl::GLboolean last_enable_blend = gl::glIsEnabled(gl::GL_BLEND);
   gl::GLboolean last_enable_cull_face = gl::glIsEnabled(gl::GL_CULL_FACE);
   gl::GLboolean last_enable_depth_test = gl::glIsEnabled(gl::GL_DEPTH_TEST);
   gl::GLboolean last_enable_scissor_test = gl::glIsEnabled(gl::GL_SCISSOR_TEST);

   // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
   gl::glEnable(gl::GL_BLEND);
   gl::glBlendEquation(gl::GL_FUNC_ADD);
   gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
   gl::glDisable(gl::GL_CULL_FACE);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glEnable(gl::GL_SCISSOR_TEST);
   gl::glActiveTexture(gl::GL_TEXTURE0);

   // Setup viewport, orthographic projection matrix
   gl::glViewport(0, 0, (gl::GLsizei)fbWidth, (gl::GLsizei)fbHeight);
   const float ortho_projection[4][4] =
   {
      { 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
      { 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
      { 0.0f,                  0.0f,                  -1.0f, 0.0f },
      { -1.0f,                  1.0f,                   0.0f, 1.0f },
   };
   gl::glUseProgram(data.shaderHandle);
   gl::glUniform1i(data.attribLocTex, 0);
   gl::glUniformMatrix4fv(data.attribLocProjMtx, 1, gl::GL_FALSE, &ortho_projection[0][0]);
   gl::glBindVertexArray(data.vaoHandle);

   for (int n = 0; n < drawData->CmdListsCount; n++)
   {
      const ImDrawList* cmdList = drawData->CmdLists[n];
      const ImDrawIdx* idxBufOffset = 0;

      gl::glBindBuffer(gl::GL_ARRAY_BUFFER, data.vboHandle);
      gl::glBufferData(gl::GL_ARRAY_BUFFER, (gl::GLsizeiptr)cmdList->VtxBuffer.size() * sizeof(ImDrawVert), (gl::GLvoid*)&cmdList->VtxBuffer.front(), gl::GL_STREAM_DRAW);

      gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, data.elementsHandle);
      gl::glBufferData(gl::GL_ELEMENT_ARRAY_BUFFER, (gl::GLsizeiptr)cmdList->IdxBuffer.size() * sizeof(ImDrawIdx), (gl::GLvoid*)&cmdList->IdxBuffer.front(), gl::GL_STREAM_DRAW);

      for (const ImDrawCmd* pcmd = cmdList->CmdBuffer.begin(); pcmd != cmdList->CmdBuffer.end(); pcmd++)
      {
         if (pcmd->UserCallback)
         {
            pcmd->UserCallback(cmdList, pcmd);
         } else
         {
            gl::glBindTexture(gl::GL_TEXTURE_2D, (gl::GLuint)(intptr_t)pcmd->TextureId);
            gl::glScissor((int)pcmd->ClipRect.x, (int)(fbHeight - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
            gl::glDrawElements(gl::GL_TRIANGLES, (gl::GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? gl::GL_UNSIGNED_SHORT : gl::GL_UNSIGNED_INT, idxBufOffset);
         }
         idxBufOffset += pcmd->ElemCount;
      }
   }

   // Restore modified GL state
   gl::glUseProgram(last_program);
   gl::glActiveTexture(static_cast<gl::GLenum>(last_active_texture));
   gl::glBindTexture(gl::GL_TEXTURE_2D, last_texture);
   gl::glBindVertexArray(last_vertex_array);
   gl::glBindBuffer(gl::GL_ARRAY_BUFFER, last_array_buffer);
   gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
   gl::glBlendEquationSeparate(static_cast<gl::GLenum>(last_blend_equation_rgb), static_cast<gl::GLenum>(last_blend_equation_alpha));
   gl::glBlendFunc(static_cast<gl::GLenum>(last_blend_src), static_cast<gl::GLenum>(last_blend_dst));
   if (last_enable_blend == gl::GL_TRUE) gl::glEnable(gl::GL_BLEND); else gl::glDisable(gl::GL_BLEND);
   if (last_enable_cull_face == gl::GL_TRUE) gl::glEnable(gl::GL_CULL_FACE); else gl::glDisable(gl::GL_CULL_FACE);
   if (last_enable_depth_test == gl::GL_TRUE) gl::glEnable(gl::GL_DEPTH_TEST); else gl::glDisable(gl::GL_DEPTH_TEST);
   if (last_enable_scissor_test == gl::GL_TRUE) gl::glEnable(gl::GL_SCISSOR_TEST); else gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glViewport(last_viewport[0], last_viewport[1], (gl::GLsizei)last_viewport[2], (gl::GLsizei)last_viewport[3]);
   gl::glScissor(last_scissor[0], last_scissor[1], last_scissor[2], last_scissor[3]);
}

} // namespace debugger

} // namespace decaf

#endif // DECAF_NOGL
