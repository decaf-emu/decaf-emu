#pragma once
#include <imgui.h>
#include <imgui_internal.h>

namespace debugger
{

namespace ui
{

static struct ImGuiPlotArrayGetterData
{
   const float* Values;
   int Stride;

   ImGuiPlotArrayGetterData() {}
   ImGuiPlotArrayGetterData(const float* values, int stride) { Values = values; Stride = stride; }
} _ImGuiPlotArrayGetterData; // prevent warning about static specifier

static float Plot_ArrayGetter(void* data, int idx)
{
   ImGuiPlotArrayGetterData* plot_data = (ImGuiPlotArrayGetterData*)data;
   const float v = *(float*)(void*)((unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
   return v;
}

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

// Extended PlotEx with plot lines, removal of histogram drawing, other tweaks
void PlotExDecaf(const char* label, float(*values_getter)(void* data, int idx), void* data,
   int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, unsigned int bg_lines);

// Wrapper for PlotExDecaf
void PlotLinesDecaf(const char* label, const float* values, int values_count, int values_offset,
   const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride, unsigned int bg_lines);

} // namespace ui

} // namespace debugger
