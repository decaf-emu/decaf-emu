#ifndef DECAF_NOGL

#include "common/gl.h"
#include "debugger/debugger_ui.h"
#include "decaf.h"
#include "decaf_debugger.h"
#include <imgui.h>
#include <gsl.h>

namespace decaf
{

namespace debugger
{

struct DebugDrawData
{
   GLuint fontTexture;
   GLuint shaderHandle;
   GLuint vertHandle;
   GLuint fragHandle;
   GLuint vboHandle;
   GLuint vaoHandle;
   GLuint elementsHandle;
   GLuint attribLocTex;
   GLuint attribLocProjMtx;
   GLuint attribLocPos;
   GLuint attribLocUV;
   GLuint attribLocColor;
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

   const GLchar *vertex_shader =
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

   const GLchar* fragment_shader =
      "#version 330\n"
      "uniform sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Color;\n"
      "out vec4 Out_Color;\n"
      "void main()\n"
      "{\n"
      "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
      "}\n";

   data.shaderHandle = glCreateProgram();
   data.vertHandle = glCreateShader(GL_VERTEX_SHADER);
   data.fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(data.vertHandle, 1, &vertex_shader, 0);
   glShaderSource(data.fragHandle, 1, &fragment_shader, 0);
   glCompileShader(data.vertHandle);
   glCompileShader(data.fragHandle);
   glAttachShader(data.shaderHandle, data.vertHandle);
   glAttachShader(data.shaderHandle, data.fragHandle);
   glLinkProgram(data.shaderHandle);

   data.attribLocTex = glGetUniformLocation(data.shaderHandle, "Texture");
   data.attribLocProjMtx = glGetUniformLocation(data.shaderHandle, "ProjMtx");
   data.attribLocPos = glGetAttribLocation(data.shaderHandle, "Position");
   data.attribLocUV = glGetAttribLocation(data.shaderHandle, "UV");
   data.attribLocColor = glGetAttribLocation(data.shaderHandle, "Color");

   glGenBuffers(1, &data.vboHandle);
   glGenBuffers(1, &data.elementsHandle);

   glGenVertexArrays(1, &data.vaoHandle);
   glBindVertexArray(data.vaoHandle);
   glBindBuffer(GL_ARRAY_BUFFER, data.vboHandle);
   glEnableVertexAttribArray(data.attribLocPos);
   glEnableVertexAttribArray(data.attribLocUV);
   glEnableVertexAttribArray(data.attribLocColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
   glVertexAttribPointer(data.attribLocPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, pos));
   glVertexAttribPointer(data.attribLocUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, uv));
   glVertexAttribPointer(data.attribLocColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

   // Upload the font texture
   unsigned char* pixels;
   int width, height;
   io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

   glGenTextures(1, &data.fontTexture);
   glBindTexture(GL_TEXTURE_2D, data.fontTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
   glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

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
   GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
   GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
   GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
   GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
   GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
   GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
   GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
   GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
   GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
   GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
   GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
   GLint last_scissor[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor);
   GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
   GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
   GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
   GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

   // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
   glEnable(GL_BLEND);
   glBlendEquation(GL_FUNC_ADD);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_SCISSOR_TEST);
   glActiveTexture(GL_TEXTURE0);

   // Setup viewport, orthographic projection matrix
   glViewport(0, 0, (GLsizei)fbWidth, (GLsizei)fbHeight);
   const float ortho_projection[4][4] =
   {
      { 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
      { 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
      { 0.0f,                  0.0f,                  -1.0f, 0.0f },
      { -1.0f,                  1.0f,                   0.0f, 1.0f },
   };
   glUseProgram(data.shaderHandle);
   glUniform1i(data.attribLocTex, 0);
   glUniformMatrix4fv(data.attribLocProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
   glBindVertexArray(data.vaoHandle);

   for (int n = 0; n < drawData->CmdListsCount; n++)
   {
      const ImDrawList* cmdList = drawData->CmdLists[n];
      const ImDrawIdx* idxBufOffset = 0;

      glBindBuffer(GL_ARRAY_BUFFER, data.vboHandle);
      glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmdList->VtxBuffer.front(), GL_STREAM_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.elementsHandle);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmdList->IdxBuffer.front(), GL_STREAM_DRAW);

      for (const ImDrawCmd* pcmd = cmdList->CmdBuffer.begin(); pcmd != cmdList->CmdBuffer.end(); pcmd++)
      {
         if (pcmd->UserCallback)
         {
            pcmd->UserCallback(cmdList, pcmd);
         } else
         {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
            glScissor((int)pcmd->ClipRect.x, (int)(fbHeight - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idxBufOffset);
         }
         idxBufOffset += pcmd->ElemCount;
      }
   }

   // Restore modified GL state
   glUseProgram(last_program);
   glActiveTexture(static_cast<GLenum>(last_active_texture));
   glBindTexture(GL_TEXTURE_2D, last_texture);
   glBindVertexArray(last_vertex_array);
   glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
   glBlendEquationSeparate(static_cast<GLenum>(last_blend_equation_rgb), static_cast<GLenum>(last_blend_equation_alpha));
   glBlendFunc(static_cast<GLenum>(last_blend_src), static_cast<GLenum>(last_blend_dst));
   if (last_enable_blend == GL_TRUE) glEnable(GL_BLEND); else glDisable(GL_BLEND);
   if (last_enable_cull_face == GL_TRUE) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
   if (last_enable_depth_test == GL_TRUE) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
   if (last_enable_scissor_test == GL_TRUE) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
   glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
   glScissor(last_scissor[0], last_scissor[1], last_scissor[2], last_scissor[3]);
}

} // namespace debugger

} // namespace decaf

#endif // DECAF_NOGL
