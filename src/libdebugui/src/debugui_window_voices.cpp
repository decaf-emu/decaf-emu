#include "debugui_window_voices.h"

#include <libdecaf/decaf_debug_api.h>
#include <imgui.h>

namespace debugui
{

static const ImVec4 CurrentColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 CurrentBgColor = HEXTOIMV4(0x00E676, 1.0f);

VoicesWindow::VoicesWindow(const std::string &name) :
   Window(name)
{
}

void
VoicesWindow::draw()
{
   ImGui::SetNextWindowSize(ImVec2 { 600, 300 }, ImGuiSetCond_FirstUseEver);

   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   ImGui::Columns(9, "voicesList", false);

   ImGui::Text("ID"); ImGui::NextColumn();
   ImGui::Text("State"); ImGui::NextColumn();
   ImGui::Text("Type"); ImGui::NextColumn();
   ImGui::Text("Strm"); ImGui::NextColumn();
   ImGui::Text("Base Addr"); ImGui::NextColumn();
   ImGui::Text("Current Off"); ImGui::NextColumn();
   ImGui::Text("End Off"); ImGui::NextColumn();
   ImGui::Text("Loop Off"); ImGui::NextColumn();
   ImGui::Text("Loop Mode"); ImGui::NextColumn();
   ImGui::Separator();

   auto voices = std::vector<decaf::debug::CafeVoice> {};
   if (decaf::debug::sampleCafeVoices(voices)) {
      for (auto &voice : voices) {
         ImGui::Text("%d", voice.index);
         ImGui::NextColumn();

         if (voice.state == decaf::debug::CafeVoice::Playing) {
            ImGui::Text("Playing");
         } else {
            ImGui::Text("Stopped");
         }
         ImGui::NextColumn();

         if (voice.format == decaf::debug::CafeVoice::ADPCM) {
            ImGui::Text("ADPCM");
         } else if (voice.format == decaf::debug::CafeVoice::LPCM16) {
            ImGui::Text("LPCM16");
         } else if (voice.format == decaf::debug::CafeVoice::LPCM8) {
            ImGui::Text("LPCM8");
         } else {
            ImGui::Text("Unknown");
         }
         ImGui::NextColumn();

         if (voice.type == decaf::debug::CafeVoice::Default) {
            ImGui::Text("Default");
         } else if (voice.type == decaf::debug::CafeVoice::Streaming) {
            ImGui::Text("Stream");
         } else {
            ImGui::Text("Unknown");
         }
         ImGui::NextColumn();

         ImGui::Text("%08x", voice.data);
         ImGui::NextColumn();

         ImGui::Text("%x", voice.currentOffset);
         ImGui::NextColumn();

         ImGui::Text("%x", voice.endOffset);
         ImGui::NextColumn();

         ImGui::Text("%x", voice.loopOffset);
         ImGui::NextColumn();

         ImGui::Text("%d", voice.loopingEnabled);
         ImGui::NextColumn();
      }
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace debugui
