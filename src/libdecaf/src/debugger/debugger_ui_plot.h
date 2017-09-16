#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <functional>

namespace debugger
{

namespace ui
{

// Extended PlotEx with plot lines, removal of histogram drawing, other tweaks
void
PlotExDecaf(const char* label,
            std::function<float(int)> valuesGetter,
            int valuesCount,
            int valuesOffset,
            const char* overlayText,
            float scaleMin,
            float scaleMax,
            ImVec2 graphSize,
            unsigned int bgLines);

// Wrapper for PlotExDecaf
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
               unsigned int bgLines);

} // namespace ui

} // namespace debugger
