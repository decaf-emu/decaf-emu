#include "debugger_ui_plot.h"

namespace debugger
{

namespace ui
{

inline ImVec2
operator+(const ImVec2 &lhs,
          const ImVec2 &rhs)
{
   return ImVec2 { lhs.x + rhs.x, lhs.y + rhs.y };
}

inline ImVec2
operator-(const ImVec2 &lhs,
          const ImVec2 &rhs)
{
   return ImVec2 { lhs.x - rhs.x, lhs.y - rhs.y };
}

inline void
PlotExDecaf(const char* label,
            std::function<float(int)> valuesGetter,
            int valuesCount,
            int valuesOffset,
            const char* overlayText,
            float scaleMin,
            float scaleMax,
            ImVec2 graphSize,
            unsigned int bgLines)
{
   ImGuiWindow *window = ImGui::GetCurrentWindow();
   if (window->SkipItems) {
      return;
   }

   ImGuiContext &g = *GImGui;
   const ImGuiStyle &style = g.Style;
   const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
   if (graphSize.x == 0.0f) {
      graphSize.x = ImGui::CalcItemWidth();
   }

   if (graphSize.y == 0.0f) {
      graphSize.y = labelSize.y + (style.FramePadding.y * 2);
   }

   const auto frameBounds = ImRect {
         window->DC.CursorPos,
         window->DC.CursorPos + ImVec2 { graphSize.x, graphSize.y }
      };
   const auto innerBounds = ImRect {
         frameBounds.Min + style.FramePadding,
         frameBounds.Max - style.FramePadding
      };
   const auto totalBounds = ImRect {
         frameBounds.Min,
         frameBounds.Max + ImVec2 { labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f, 0 }
      };

   ImGui::ItemSize(totalBounds, style.FramePadding.y);
   if (!ImGui::ItemAdd(totalBounds, NULL)) {
      return;
   }

   // Determine scale from values if not specified
   if (scaleMin == FLT_MAX || scaleMax == FLT_MAX) {
      float v_min = FLT_MAX;
      float v_max = -FLT_MAX;

      for (int i = 0; i < valuesCount; i++) {
         const float v = valuesGetter(i);
         v_min = ImMin(v_min, v);
         v_max = ImMax(v_max, v);
      }

      if (scaleMin == FLT_MAX) {
         scaleMin = v_min;
      }

      if (scaleMax == FLT_MAX) {
         scaleMax = v_max;
      }
   }

   ImGui::RenderFrame(frameBounds.Min,
                      frameBounds.Max,
                      ImGui::GetColorU32(ImGuiCol_FrameBg),
                      true, style.FrameRounding);

   if (valuesCount > 0) {
      int resW = ImMin((int)graphSize.x, valuesCount) - 1;
      int itemCount = valuesCount -1;

      // Tooltip on hover
      int vHovered = -1;
      if (ImGui::IsHovered(innerBounds, 0)) {
         const float t = ImClamp((g.IO.MousePos.x - innerBounds.Min.x) /
                         (innerBounds.Max.x - innerBounds.Min.x), 0.0f, 0.9999f);
         const int vIdx = (int)(t * itemCount);
         IM_ASSERT(vIdx >= 0 && vIdx < valuesCount);

         const float v0 = valuesGetter((vIdx + valuesOffset) % valuesCount);
         const float v1 = valuesGetter((vIdx + 1 + valuesOffset) % valuesCount);
         ImGui::SetTooltip("%8.4g", v1);
         vHovered = vIdx;
      }

      const auto tStep = 1.0f / (float)resW;
      auto v0 = valuesGetter((0 + valuesOffset) % valuesCount);
      auto t0 = 0.0f;

      // Point in the normalized space of our target rectangle
      auto tp0 = ImVec2 { t0, 1.0f - ImSaturate((v0 - scaleMin) / (scaleMax - scaleMin)) };

      static const ImU32 colBase = ImGui::GetColorU32(ImGuiCol_PlotLines);
      static const ImU32 colHovered = ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
      static const ImU32 colBgLines = IM_COL32(255, 255, 255, 25);

      for (int n = 0; n < resW; n++) {
         const float t1 = t0 + tStep;
         const int v1Idx = (int)(t0 * itemCount + 0.5f);
         IM_ASSERT(v1Idx >= 0 && v1Idx < valuesCount);
         const float v1 = valuesGetter((v1Idx + valuesOffset + 1) % valuesCount);
         const ImVec2 tp1 = ImVec2 { t1, 1.0f - ImSaturate((v1 - scaleMin) / (scaleMax - scaleMin)) };

         // NB: Draw calls are merged together by the DrawList system.
         // Still, we should render our batch are lower level to save a bit of CPU.
         ImVec2 pos0 = ImLerp(innerBounds.Min, innerBounds.Max, tp0);
         ImVec2 pos1 = ImLerp(innerBounds.Min, innerBounds.Max, tp1);
         window->DrawList->AddLine(pos0, pos1, vHovered == v1Idx ? colHovered : colBase);

         t0 = t1;
         tp0 = tp1;
      }

      // Background plot lines
      const auto innerHeight = innerBounds.GetHeight();
      const auto bgLineSpacing = innerHeight / bgLines;

      for (float yOffset = innerHeight; yOffset >= 0; yOffset -= bgLineSpacing) {
         window->DrawList->AddLine(
            innerBounds.Min + ImVec2{ 0, yOffset },
            innerBounds.GetTR() + ImVec2{ 0, yOffset },
            colBgLines);
      }
   }

   // Text overlay
   if (overlayText) {
      ImGui::RenderTextClipped(ImVec2 { frameBounds.Min.x, frameBounds.Min.y + style.FramePadding.y },
                               frameBounds.Max,
                               overlayText,
                               NULL, NULL, ImVec2(0.5f, 0.0f));
   }

   if (labelSize.x > 0.0f) {
      ImGui::RenderText(ImVec2 { frameBounds.Max.x + style.ItemInnerSpacing.x, innerBounds.Min.y },
                        label);
   }
}

void
PlotLinesDecaf(const char *label,
               const float *values,
               int valuesCount,
               int valuesOffset,
               const char *overlayText,
               float scaleMin,
               float scaleMax,
               ImVec2 graphSize,
               int stride,
               unsigned int bgLines)
{
   auto valuesGetter = [&](int idx) {
      return *reinterpret_cast<const float *>(
         reinterpret_cast<const uint8_t *>(values) + stride * idx);
   };

   PlotExDecaf(label, valuesGetter, valuesCount, valuesOffset, overlayText,
               scaleMin, scaleMax, graphSize, bgLines);
}

} // namespace ui

} // namespace debugger
