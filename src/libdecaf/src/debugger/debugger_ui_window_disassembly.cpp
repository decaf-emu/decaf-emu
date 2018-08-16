#include "decaf_input.h"
#include "debugger_analysis.h"
#include "debugger_branchcalc.h"
#include "debugger_threadutils.h"
#include "debugger_ui_window_disassembly.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"

#include <algorithm>
#include <libcpu/cpu_breakpoints.h>
#include <libcpu/espresso/espresso_disassembler.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libcpu/mem.h>
#include <libcpu/mmu.h>
#include <libcpu/state.h>
#include <imgui.h>
#include <map>
#include <sstream>

namespace debugger
{

namespace ui
{

static const ImVec4 DataColor = HEXTOIMV4(0xB0C4CE, 1.0f);
static const ImVec4 FuncColor = HEXTOIMV4(0xAB47BC, 1.0f);
static const ImVec4 FuncLinkColor = HEXTOIMV4(0xCFD8DC, 1.0f);
static const ImVec4 FuncFollowColor = HEXTOIMV4(0x66BB6A, 1.0f);
static const ImVec4 FuncSkipColor = HEXTOIMV4(0xF44336, 1.0f);
static const ImVec4 JmpColor = HEXTOIMV4(0xFF5722, 1.0f);
static const ImVec4 SelBgColor = HEXTOIMV4(0x263238, 1.0f);
static const ImVec4 NiaColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 NiaBgColor = HEXTOIMV4(0x00E676, 1.0f);
static const ImVec4 BpColor = HEXTOIMV4(0x000000, 1.0f);
static const ImVec4 BpBgColor = HEXTOIMV4(0xF44336, 1.0f);

enum class BranchGlyph
{
   None,
   StartDown,
   StartUp,
   EndDown,
   EndUp,
   Middle,
   EndBoth,
};

struct VisInstrInfo
{
   BranchGlyph branchGlyph;
   ImVec4 branchColor;
};

DisassemblyWindow::DisassemblyWindow(const std::string &name) :
   Window(name)
{
}

void
DisassemblyWindow::gotoAddress(uint32_t address)
{
   mAddressScroller.scrollTo(address);
   mSelectedAddr = address;
}

void
DisassemblyWindow::draw()
{
   if (!ImGui::Begin(mName.c_str(), &mVisible)) {
      ImGui::End();
      return;
   }

   auto activeThread = mStateTracker->getActiveThread();
   auto activeCoreRegs = getThreadCoreContext(mDebugger, activeThread);

   // Check if we need to move around or scroll or mark stuff
   if (mSelectedAddr != -1) {
      auto originalAddress = mSelectedAddr;

      // Check if the user tapped F, if so, mark this as a function!
      if (ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F))) {
         analysis::toggleAsFunction(static_cast<uint32_t>(mSelectedAddr));
      }

      // Check if the user tapped Enter, if so, jump to the branch target!
      if (ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::Enter))) {
         auto selectedAddr = static_cast<uint32_t>(mSelectedAddr);

         if (cpu::isValidAddress(cpu::VirtualAddress { selectedAddr })) {
            auto instr = cpu::getBreakpointSavedCode(selectedAddr);
            auto data = espresso::decodeInstruction(instr);

            if (data && analysis::isBranchInstr(data)) {
               auto meta = analysis::getBranchMeta(selectedAddr, instr, data, activeCoreRegs);

               if (!meta.isVariable || activeCoreRegs) {
                  mSelectedAddr = meta.target;
               }
            }
         }
      }

      // Check if the user wants to move
      if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
         mSelectedAddr -= 4;
      } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
         mSelectedAddr += 4;
      }

      // Clamp the edit address
      mSelectedAddr = std::max<int64_t>(0, mSelectedAddr);
      mSelectedAddr = std::min<int64_t>(mSelectedAddr, 0xFFFFFFFF);

      // Before we start processing an edit, lets make sure it's valid memory to be editing...
      if (!cpu::isValidAddress(cpu::VirtualAddress { static_cast<uint32_t>(mSelectedAddr) })) {
         mSelectedAddr = -1;
      }

      // Make sure that the address always stays visible!  We do this before
      //  checking for valid memory so you can still goto an invalid address.
      if (mSelectedAddr != originalAddress && mSelectedAddr != -1) {
         mAddressScroller.scrollTo(static_cast<uint32_t>(mSelectedAddr));
      }
   }

   // We use this 'hack' to get the true with without line-advance offsets.
   auto lineHeight = ImGui::GetTextLineHeight();
   auto glyphWidth = ImGui::CalcTextSize("FF").x - ImGui::CalcTextSize("F").x;

   // We precalcalculate
   auto addrAdvance = glyphWidth * 10.0f;
   auto dataAdvance = glyphWidth * 9.0f;
   auto funcLineAdvance = glyphWidth * 1.0f;
   auto jmpArrowAdvance = glyphWidth * 1.0f;
   auto jmpLineAdvance = glyphWidth * 1.0f;
   auto instrAdvance = glyphWidth * 28.0f;

   // We grab some stuff to do some custom rendering...
   auto drawList = ImGui::GetWindowDrawList();
   auto wndWidth = ImGui::GetWindowContentRegionWidth();

   // Lets precalculate some stuff we need for the currently visible instructions
   std::map<uint32_t, VisInstrInfo> visInstrInfo;
   mAddressScroller.begin(4, ImVec2 { 0, -ImGui::GetItemsLineHeightWithSpacing() });

   // Find the upper and lower bounds of the visible area
   auto visFirstAddr = mAddressScroller.reset();
   auto visLastAddr = visFirstAddr;

   for (auto addr = visFirstAddr; mAddressScroller.hasMore(); addr = mAddressScroller.advance()) {
      visLastAddr = addr;
   }

   auto ForEachVisInstr = [&](uint32_t first, uint32_t last, std::function<void(uint32_t, VisInstrInfo&)> fn) {
      if (first > last) {
         std::swap(first, last);
      }

      auto visLoopFirst = std::max(first, visFirstAddr);
      auto visLoopLast = std::min(last, visLastAddr);

      if (visLoopFirst > visLastAddr || visLoopLast < visFirstAddr) {
         return;
      }

      for (auto addr = visLoopFirst; ; addr += 4) {
         fn(addr, visInstrInfo[addr]);

         if (addr == visLoopLast) {
            break;
         }
      }
   };

   if (mSelectedAddr != -1) {
      auto selectedAddr = static_cast<uint32_t>(mSelectedAddr);
      auto instr = cpu::getBreakpointSavedCode(selectedAddr);
      auto data = espresso::decodeInstruction(instr);
      auto info = analysis::get(selectedAddr);
      auto isVisBranchSource = false;

      if (data && analysis::isBranchInstr(data)) {
         auto meta = analysis::getBranchMeta(selectedAddr, instr, data, activeCoreRegs);

         if (!meta.isCall && (!meta.isVariable || activeCoreRegs)) {
            ForEachVisInstr(selectedAddr, meta.target,
                            [&](uint32_t addr, VisInstrInfo &vii) {
               if (addr == selectedAddr) {
                  if (meta.target < selectedAddr) {
                     vii.branchGlyph = BranchGlyph::StartUp;
                  } else {
                     vii.branchGlyph = BranchGlyph::StartDown;
                  }
               } else if (addr == meta.target) {
                  if (meta.target < selectedAddr) {
                     vii.branchGlyph = BranchGlyph::EndUp;
                  } else {
                     vii.branchGlyph = BranchGlyph::EndDown;
                  }
               } else {
                  vii.branchGlyph = BranchGlyph::Middle;
               }

               if (activeCoreRegs && meta.conditionSatisfied) {
                  vii.branchColor = FuncFollowColor;;
               } else if (activeCoreRegs && !meta.conditionSatisfied) {
                  vii.branchColor = FuncSkipColor;
               } else {
                  vii.branchColor = FuncLinkColor;
               }
            });

            isVisBranchSource = true;
         }
      }

      if (!isVisBranchSource) {
         if (info.instr) {
            auto &visInfo = visInstrInfo[selectedAddr];
            auto selectedMinSource = selectedAddr;
            auto selectedMaxSource = selectedAddr;

            for (auto sourceAddr : info.instr->sourceBranches) {
               if (sourceAddr > selectedMaxSource) {
                  selectedMaxSource = sourceAddr;
               }

               if (sourceAddr < selectedMinSource) {
                  selectedMinSource = sourceAddr;
               }

               auto &sourceVisInfo = visInstrInfo[sourceAddr];

               if (sourceAddr > selectedAddr) {
                  sourceVisInfo.branchGlyph = BranchGlyph::StartUp;
               } else {
                  sourceVisInfo.branchGlyph = BranchGlyph::StartDown;
               }

               sourceVisInfo.branchColor = FuncLinkColor;
            }

            if (selectedMinSource < selectedAddr && selectedMaxSource > selectedAddr) {
               visInfo.branchGlyph = BranchGlyph::EndBoth;
            } else if (selectedMinSource < selectedAddr) {
               visInfo.branchGlyph = BranchGlyph::EndDown;
            } else if (selectedMaxSource > selectedAddr) {
               visInfo.branchGlyph = BranchGlyph::EndUp;
            }

            if (selectedMinSource != selectedMaxSource) {
               ForEachVisInstr(selectedMinSource, selectedMaxSource,
                               [&](uint32_t addr, VisInstrInfo &vii) {
                  if (vii.branchGlyph == BranchGlyph::None) {
                     vii.branchGlyph = BranchGlyph::Middle;
                     vii.branchColor = FuncLinkColor;
                  }
               });
            }

            visInfo.branchColor = FuncLinkColor;
         }
      }
   }

   for (auto addr = mAddressScroller.reset(); mAddressScroller.hasMore(); addr = mAddressScroller.advance()) {
      auto rootPos = ImGui::GetCursorScreenPos();
      auto linePos = ImGui::GetCursorPos();

      auto lineMin = ImVec2 { rootPos.x, rootPos.y };
      auto lineMax = ImVec2 { rootPos.x + wndWidth, rootPos.y + lineHeight };

      // Handle a new address being selected
      if (ImGui::IsMouseHoveringRect(lineMin, lineMax)) {
         if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDown(0)) {
            mSelectedAddr = addr;
         }
         // Maybe render a 'highlight' here?
      }

      // Draw the rectangle for the current line selection
      if (static_cast<int64_t>(addr) == mSelectedAddr) {
         auto lineMinB = ImVec2 { lineMin.x - 1, lineMin.y };
         auto lineMaxB = ImVec2 { lineMax.x + 1, lineMax.y };
         drawList->AddRectFilled(lineMinB, lineMaxB, ImColor { SelBgColor }, 2.0f);
      }

      // Check if this is the current instruction
      // This should be simpler...  gActiveCoreIdx instead maybe?
      auto hasBreakpoint = cpu::hasBreakpoint(addr);

      if (activeThread && addr == getThreadNia(mDebugger, activeThread)) {
         // Render a green BG behind the address
         auto lineMinC = ImVec2 { lineMin.x - 1, lineMin.y };
         auto lineMaxC = ImVec2 { lineMin.x + (glyphWidth * 8) + 2, lineMax.y };
         drawList->AddRectFilled(lineMinC, lineMaxC, ImColor { NiaBgColor }, 2.0f);

         // Render the address for this line
         //  (custom colored text so it is easy to see)
         ImGui::TextColored(NiaColor, "%08X:", addr);
      } else if (hasBreakpoint) {
         // Render a red BG behind the address
         auto lineMinC = ImVec2 { lineMin.x - 1, lineMin.y };
         auto lineMaxC = ImVec2 { lineMin.x + (glyphWidth * 8) + 2, lineMax.y };
         drawList->AddRectFilled(lineMinC, lineMaxC, ImColor { BpBgColor }, 2.0f);

         // Render the address for this line
         //  (custom colored text so it is easy to see)
         ImGui::TextColored(BpColor, "%08X:", addr);
      } else {
         // Render the address for this line in normal color
         ImGui::Text("%08X:", addr);
      }

      if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
         if (mDebugger->hasBreakpoint(addr)) {
            mDebugger->removeBreakpoint(addr);
         } else {
            mDebugger->addBreakpoint(addr);
         }
      }

      linePos.x += addrAdvance;

      // Stop drawing if this is invalid memory
      if (!cpu::isValidAddress(cpu::VirtualAddress { addr })) {
         continue;
      }

      auto instr = cpu::getBreakpointSavedCode(addr);
      auto data = espresso::decodeInstruction(instr);
      auto info = analysis::get(addr);

      auto visInfoIter = visInstrInfo.find(addr);
      auto *visInfo = visInfoIter != visInstrInfo.end() ? &visInfoIter->second : nullptr;

      // Render the instructions bytes
      ImGui::SetCursorPos(linePos);
      ImGui::TextColored(DataColor, "%02x%02x%02x%02x",
                         (instr >> 24) & 0xff,
                         (instr >> 16) & 0xff,
                         (instr >> 8) & 0xff,
                         (instr >> 0) & 0xff);

      linePos.x += dataAdvance;

      if (info.func) {
         ImGui::SetCursorPos(linePos);

         if (addr == info.func->start) {
            ImGui::TextColored(FuncColor, u8"\u250f");
         } else if (info.func->end == 0xFFFFFFFF) {
            if (addr == info.func->start + 4) {
               ImGui::TextColored(FuncColor, u8"\u2575");
               ImGui::SetCursorPos(linePos);
               ImGui::TextColored(FuncColor, u8"\u25BE");
            }
         } else if (addr == info.func->end - 4) {
            ImGui::TextColored(FuncColor, u8"\u2517");
         } else {
            ImGui::TextColored(FuncColor, u8"\u2503");
         }
      }

      linePos.x += funcLineAdvance;

      // This renders an arrow representing the direction of any branch statement
      if (data && analysis::isBranchInstr(data)) {
         auto meta = analysis::getBranchMeta(addr, instr, data, nullptr);
         if (!meta.isVariable && !meta.isCall) {
            ImGui::SetCursorPos(linePos);
            if (meta.target > addr) {
               ImGui::TextColored(JmpColor, u8"\u25BE");
            } else {
               ImGui::TextColored(JmpColor, u8"\u25B4");
            }
         }
      }

      linePos.x += jmpArrowAdvance;

      // This renders the brackets which display jump source/destinations
      //  for the currently selected instruction
      if (visInfo) {
         auto selectedGlyph = visInfo->branchGlyph;
         auto selectedColor = visInfo->branchColor;

         if (selectedGlyph != BranchGlyph::None) {
            // This drawing is a bit of a hack to be honest, but it looks nicer...
            ImGui::SetCursorPos(ImVec2 { linePos.x - glyphWidth*0.25f, linePos.y });

            if (selectedGlyph == BranchGlyph::StartDown) {
               ImGui::TextColored(selectedColor, u8"\u256D");
            } else if (selectedGlyph == BranchGlyph::StartUp) {
               ImGui::TextColored(selectedColor, u8"\u2570");
            } else if (selectedGlyph == BranchGlyph::Middle) {
               ImGui::TextColored(selectedColor, u8"\u2502");
            } else if (selectedGlyph == BranchGlyph::EndDown) {
               ImGui::TextColored(selectedColor, u8"\u2570");
               ImGui::SetCursorPos(ImVec2 { linePos.x + glyphWidth*0.25f, linePos.y });
               ImGui::TextColored(selectedColor, u8"\u25B8");
            } else if (selectedGlyph == BranchGlyph::EndUp) {
               ImGui::TextColored(selectedColor, u8"\u256D");
               ImGui::SetCursorPos(ImVec2 { linePos.x + glyphWidth*0.25f, linePos.y });
               ImGui::TextColored(selectedColor, u8"\u25B8");
            } else if (selectedGlyph == BranchGlyph::EndBoth) {
               ImGui::TextColored(selectedColor, u8"\u251C");
            }
         }
      }

      linePos.x += jmpLineAdvance;
      ImGui::SetCursorPos(linePos);

      espresso::Disassembly dis;

      if (espresso::disassemble(instr, dis, addr)) {
         // TODO: Better integration with the disassembler,
         //  as well as providing per-arg highlighting?
         auto cmdVsArgs = dis.text.find(' ');
         if (cmdVsArgs != std::string::npos) {
            auto cmd = dis.text.substr(0, cmdVsArgs);
            auto args = dis.text.substr(cmdVsArgs + 1);
            ImGui::Text("%-6s %s", cmd.c_str(), args.c_str());
         } else {
            ImGui::Text("%-6s", dis.text.c_str());
         }
      } else {
         ImGui::Text("??");
      }

      linePos.x += instrAdvance;

      std::string lineInfo;

      if (info.func && addr == info.func->start && info.func->name.size() > 0) {
         lineInfo = info.func->name;
      }

      if (data && analysis::isBranchInstr(data)) {
         auto meta = analysis::getBranchMeta(addr, instr, data, nullptr);
         if (!meta.isVariable) {
            auto func = analysis::getFunction(meta.target);
            if (func) {
               if (lineInfo.size() > 0) {
                  lineInfo += " - ";
               }
               lineInfo += "<" + func->name + ">";
            }
         }
      }

      if (info.instr && info.instr->comments.size() > 0) {
         if (lineInfo.size() > 0) {
            lineInfo += " - ";
         }
         lineInfo += info.instr->comments;
      }

      if (lineInfo.size() > 0) {
         ImGui::SetCursorPos(linePos);
         ImGui::Text("; %s", lineInfo.c_str());
      }
   }
   mAddressScroller.end();

   ImGui::Separator();

   // Render the bottom bar for the window
   ImGui::AlignFirstTextHeightToWidgets();
   ImGui::Text("Go To Address: ");
   ImGui::SameLine();
   ImGui::PushItemWidth(70);

   char addressInput[32];
   if (ImGui::InputText("##addr", addressInput, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
      std::istringstream is { addressInput };
      uint32_t goto_addr;

      if ((is >> std::hex >> goto_addr)) {
         gotoAddress(goto_addr);
      }
   }

   ImGui::PopItemWidth();
   ImGui::End();
}

} // namespace ui

} // namespace debugger
