#include "debugger_ui_window_voices.h"
#include "cafe/libraries/sndcore2/sndcore2_enum.h"
#include "cafe/libraries/sndcore2/sndcore2_voice.h"

#include <imgui.h>

namespace debugger
{

namespace ui
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

   auto voices = cafe::sndcore2::internal::getAcquiredVoices();

   for (auto &voice : voices) {
      auto extras = cafe::sndcore2::internal::getVoiceExtras(voice->index);

      ImGui::Text("%d", voice->index.value());
      ImGui::NextColumn();

      if (voice->state == cafe::sndcore2::AXVoiceState::Playing) {
         ImGui::Text("Playing");
      } else {
         ImGui::Text("Stopped");
      }
      ImGui::NextColumn();

      if (voice->offsets.dataType == cafe::sndcore2::AXVoiceFormat::ADPCM) {
         ImGui::Text("ADPCM");
      } else if (voice->offsets.dataType == cafe::sndcore2::AXVoiceFormat::LPCM16) {
         ImGui::Text("LPCM16");
      } else if (voice->offsets.dataType == cafe::sndcore2::AXVoiceFormat::LPCM8) {
         ImGui::Text("LPCM8");
      } else {
         ImGui::Text("Unknown");
      }
      ImGui::NextColumn();

      if (extras->type == cafe::sndcore2::AXVoiceType::Default) {
         ImGui::Text("Default");
      } else if (extras->type == cafe::sndcore2::AXVoiceType::Streaming) {
         ImGui::Text("Stream");
      } else {
         ImGui::Text("Unknown");
      }
      ImGui::NextColumn();

      ImGui::Text("%08x", virt_cast<virt_addr>(voice->offsets.data).getAddress());
      ImGui::NextColumn();

      ImGui::Text("%x", voice->offsets.currentOffset.value());
      ImGui::NextColumn();

      ImGui::Text("%x", voice->offsets.endOffset.value());
      ImGui::NextColumn();

      ImGui::Text("%x", voice->offsets.loopOffset.value());
      ImGui::NextColumn();

      ImGui::Text("%d", voice->offsets.loopingEnabled.value());
      ImGui::NextColumn();
   }

   ImGui::Columns(1);
   ImGui::End();
}

} // namespace ui

} // namespace debugger
