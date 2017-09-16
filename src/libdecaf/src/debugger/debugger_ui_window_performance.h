#pragma once
#include "debugger_ui_window.h"
#include <array>

namespace debugger
{

namespace ui
{

class PerformanceWindow : public Window
{
   static constexpr auto GraphHeight = 70.0f;
   static constexpr auto NumFpsSamples = 100u;
   static constexpr auto NumFtSamples = 100u;

public:
   PerformanceWindow(const std::string &name);
   virtual ~PerformanceWindow() = default;

   void draw() override;
   void drawGraphs();

   static PerformanceWindow* create(const std::string &name);

private:
   std::array<float, NumFpsSamples> mFpsValues;
   std::array<float, NumFtSamples> mFtValues;
};

} // namespace ui

} // namespace debugger
