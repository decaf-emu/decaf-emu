#include "debugger_ui_plot.h"

namespace debugger
{

namespace ui
{

inline void
PlotExDecaf(const char* label, float(*values_getter)(void* data, int idx), void* data,
   int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size,
   unsigned int bg_lines)
{
   ImGuiWindow* window = ImGui::GetCurrentWindow();
   if (window->SkipItems)
      return;

   ImGuiContext& g = *GImGui;
   const ImGuiStyle& style = g.Style;

   const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
   if (graph_size.x == 0.0f)
      graph_size.x = ImGui::CalcItemWidth();
   if (graph_size.y == 0.0f)
      graph_size.y = label_size.y + (style.FramePadding.y * 2);

   const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
   const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
   const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));

   ImGui::ItemSize(total_bb, style.FramePadding.y);
   if (!ImGui::ItemAdd(total_bb, NULL))
      return;

   // Determine scale from values if not specified
   if (scale_min == FLT_MAX || scale_max == FLT_MAX)
   {
      float v_min = FLT_MAX;
      float v_max = -FLT_MAX;
      for (int i = 0; i < values_count; i++)
      {
         const float v = values_getter(data, i);
         v_min = ImMin(v_min, v);
         v_max = ImMax(v_max, v);
      }
      if (scale_min == FLT_MAX)
         scale_min = v_min;
      if (scale_max == FLT_MAX)
         scale_max = v_max;
   }

   ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

   if (values_count > 0)
   {
      int res_w = ImMin((int)graph_size.x, values_count) - 1;
      int item_count = values_count -1;

      // Tooltip on hover
      int v_hovered = -1;
      if (ImGui::IsHovered(inner_bb, 0))
      {
         const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
         const int v_idx = (int)(t * item_count);
         IM_ASSERT(v_idx >= 0 && v_idx < values_count);

         const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
         const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
         ImGui::SetTooltip("%8.4g", v1);
         v_hovered = v_idx;
      }

      const float t_step = 1.0f / (float)res_w;

      float v0 = values_getter(data, (0 + values_offset) % values_count);
      float t0 = 0.0f;
      ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min)));    // Point in the normalized space of our target rectangle

      static const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotLines);
      static const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
      static const ImU32 col_bg_lines = IM_COL32(255, 255, 255, 25);

      for (int n = 0; n < res_w; n++)
      {
         const float t1 = t0 + t_step;
         const int v1_idx = (int)(t0 * item_count + 0.5f);
         IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
         const float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
         const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)));

         // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
         ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
         ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);
         window->DrawList->AddLine(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);

         t0 = t1;
         tp0 = tp1;
      }

      // Background plot lines
      const auto innerHeight = inner_bb.GetHeight();
      const auto bg_line_spacing = innerHeight / bg_lines;

      for (float yOffset = innerHeight; yOffset >= 0; yOffset -= bg_line_spacing) {
         window->DrawList->AddLine(
            inner_bb.Min + ImVec2{ 0, yOffset },
            inner_bb.GetTR() + ImVec2{ 0, yOffset },
            col_bg_lines);
      }
   }
   // Text overlay
   if (overlay_text)
      ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

   if (label_size.x > 0.0f)
      ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
}

void
PlotLinesDecaf(const char* label, const float* values, int values_count, int values_offset,
   const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride, unsigned int bg_lines)
{
   ImGuiPlotArrayGetterData data(values, stride);
   PlotExDecaf(label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text,scale_min, scale_max, graph_size, bg_lines);
}

} // namespace ui

} // namespace debugger
