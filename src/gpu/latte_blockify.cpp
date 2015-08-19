#include "latte_shadir.h"
#include "latte.h"
#include "bitutils.h"
#include <cassert>
#include <set>
#include <stack>
#include <functional>
#include <algorithm>
#include <iostream>

namespace latte
{

struct Label
{
   enum Type
   {
      LoopStart,
      LoopEnd,
      ConditionalStart,
      ConditionalElse,
      ConditionalEnd,
      Eliminated
   };

   Label(Type type, shadir::Instruction *ins) :
      type(type), first(ins)
   {
   }

   Type type;
   shadir::Instruction *first = nullptr;
   std::vector<shadir::Block*> *restoreBlockList = nullptr;
   Label *linkedLabel = nullptr;
   shadir::Block *linkedBlock = nullptr;
};

static bool labelify(Shader &shader, std::vector<Label *> &labels);
static bool blockify(Shader &shader, std::vector<Label *> labels);

static void printBlockList(std::vector<shadir::Block*> &blocks)
{
   for (auto block : blocks) {
      if (block->type == shadir::Block::CodeBlock) {
         auto codeBlock = reinterpret_cast<shadir::CodeBlock*>(block);

         for (auto ins : codeBlock->code) {
            std::cout << ins->cfPC << " " << ins->name << std::endl;
         }
      } else if (block->type == shadir::Block::Loop) {
         auto loopBlock = reinterpret_cast<shadir::LoopBlock*>(block);
         std::cout << "while(true) { // START_LOOP " << std::endl;
         printBlockList(loopBlock->inner);
         std::cout << "} // END_LOOP " << std::endl;
      } else if (block->type == shadir::Block::Conditional) {
         auto condBlock = reinterpret_cast<shadir::ConditionalBlock*>(block);
         std::cout << "if(" << condBlock->condition->cfPC << " " << condBlock->condition->name << ") {" << std::endl;
         printBlockList(condBlock->inner);

         if (condBlock->innerElse.size()) {
            std::cout << "} else {" << std::endl;
            printBlockList(condBlock->innerElse);
         }

         std::cout << "} // END_COND " << std::endl;
      }
   }
}

bool blockify(Shader &shader)
{
   std::vector<Label *> labels;

   auto result = labelify(shader, labels);

   if (result) {
      result = blockify(shader, labels);
   }

   // Ewwww clean up labels
   for (auto label : labels) {
      delete label;
   }

   // Debug: Print block list
   printBlockList(shader.blocks);

   return result;
}

static bool labelify(Shader &shader, std::vector<Label *> &labels)
{
   std::stack<shadir::CfInstruction *> loopStarts; // LOOP_START*
   std::stack<shadir::CfInstruction *> pushes;     // PUSH
   std::stack<shadir::CfInstruction *> pops;       // POP*
   std::stack<shadir::AluInstruction *> predSets;  // PRED_SET*

   // Iterate over the code and find matching code patterns to labelify
   for (auto itr = shader.code.begin(); itr != shader.code.end(); ++itr) {
      auto ins = *itr;

      if (ins->insType == shadir::Instruction::ControlFlow) {
         auto cfIns = reinterpret_cast<shadir::CfInstruction*>(ins);

         if (cfIns->id == latte::cf::inst::LOOP_START
             || cfIns->id == latte::cf::inst::LOOP_START_DX10
             || cfIns->id == latte::cf::inst::LOOP_START_NO_AL) {
            // Found a loop start, find a matching loop_end in the correct stack
            loopStarts.push(cfIns);
         } else if (cfIns->id == latte::cf::inst::LOOP_END) {
            assert(loopStarts.size());

            // Create a LoopStart label
            auto labelStart = new Label { Label::LoopStart, loopStarts.top() };
            loopStarts.pop();
            labels.push_back(labelStart);

            // Create a LoopEnd label
            auto labelEnd = new Label { Label::LoopEnd, ins };
            labels.push_back(labelEnd);

            // Link the start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == latte::cf::inst::LOOP_CONTINUE || cfIns->id == latte::cf::inst::LOOP_BREAK) {
            assert(loopStarts.size());
            assert(predSets.size());

            // Create a ConditionalStart label for the last PRED_SET
            auto labelStart = new Label { Label::ConditionalStart, predSets.top() };
            predSets.pop();
            labels.push_back(labelStart);

            // Create a ConditionalEnd label for after the BREAK/CONTINUE instruction
            auto labelEnd = new Label { Label::ConditionalEnd, *(itr + 1) };
            labels.push_back(labelEnd);

            // Link the start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == latte::cf::inst::JUMP) {
            assert(predSets.size());
            assert(pushes.size());

            // Find the end of the jump
            auto jumpEnd = std::find_if(itr, shader.code.end(),
                                        [cfIns](auto &ins) {
                                           return ins->cfPC == cfIns->addr;
                                        });

            assert(jumpEnd != shader.code.end());

            // Check if this jump has an ELSE branch
            auto jumpElse = shader.code.end();

            if ((*jumpEnd)->insType == shadir::Instruction::ControlFlow) {
               auto jumpEndCfIns = reinterpret_cast<shadir::CfInstruction*>(*jumpEnd);

               if (jumpEndCfIns->id == latte::cf::inst::ELSE) {
                  jumpElse = jumpEnd;

                  // Find the real end of the jump
                  jumpEnd = std::find_if(jumpElse, shader.code.end(),
                                         [jumpEndCfIns](auto &ins) {
                                            return ins->cfPC == jumpEndCfIns->addr;
                                         });

                  assert(jumpEnd != shader.code.end());
               }
            }

            // Create a conditional start label
            auto labelStart = new Label { Label::ConditionalStart, predSets.top() };
            predSets.pop();
            labels.push_back(labelStart);

            // Create a conditional else label, if needed
            if (jumpElse != shader.code.end()) {
               auto labelElse = new Label { Label::ConditionalElse, *jumpElse };
               labelElse->linkedLabel = labelStart;
               labels.push_back(labelElse);
            }

            // Create a conditional end label
            auto labelEnd = new Label { Label::ConditionalEnd, *jumpEnd };
            labels.push_back(labelEnd);

            // Eliminate our JUMP instruction
            labels.push_back(new Label { Label::Eliminated, ins });

            // Link start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == latte::cf::inst::PUSH) {
            pushes.push(cfIns);
         } else if (cfIns->id == latte::cf::inst::POP) {
            pops.push(cfIns);
         }
      } else if (ins->insType == shadir::Instruction::ALU) {
         auto aluIns = reinterpret_cast<shadir::AluInstruction *>(ins);

         if (aluIns->opType == latte::alu::Encoding::OP2) {
            auto &opcode = latte::alu::op2info[aluIns->op2];

            if (opcode.flags & latte::alu::Opcode::PredSet) {
               predSets.push(aluIns);
            }
         }
      }
   }

   // Lets be sure we consumed everything we are interested in!
   assert(loopStarts.size() == 0);
   assert(predSets.size() == 0);

   // Sort the labels
   std::sort(labels.begin(), labels.end(),
             [](auto lhs, auto rhs) {
      return lhs->first->cfPC < rhs->first->cfPC;
   });

   return true;
}

static bool blockify(Shader &shader, std::vector<Label *> labels)
{
   shadir::CodeBlock *activeCodeBlock = nullptr;
   auto activeBlockList = &shader.blocks;
   auto labelItr = labels.begin();
   Label *label = nullptr;

   if (labelItr != labels.end()) {
      label = *labelItr;
   }

   // Iterate over code and find matching labels to generate code blocks
   for (auto &ins : shader.code) {
      bool insertToCode = true;

      if (label && label->first == ins) {
         // Most labels will skip current instruction
         insertToCode = false;

         if (label->type == Label::LoopStart) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::LoopEnd);

            // Save the active block list to the LoopEnd label
            label->linkedLabel->restoreBlockList = activeBlockList;

            // Create a new loop block
            auto loopBlock = new shadir::LoopBlock;
            label->linkedBlock = loopBlock;
            activeBlockList->push_back(loopBlock);

            // Set the current block list to the loop inner
            activeBlockList = &loopBlock->inner;
            activeCodeBlock = nullptr;
         } else if (label->type == Label::LoopEnd) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::LoopStart);
            assert(label->restoreBlockList);

            // Get the matching LoopBlock from the LoopStart label
            auto loopBlock = reinterpret_cast<shadir::LoopBlock *>(label->linkedLabel->linkedBlock);

            // Restore the previous block list
            activeBlockList = label->restoreBlockList;
            activeCodeBlock = nullptr;
         } else if (label->type == Label::ConditionalStart) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::ConditionalEnd);

            // Save the active block list to the ConditionalEnd label
            label->linkedLabel->restoreBlockList = activeBlockList;

            // Create a new conditional block
            auto condBlock = new shadir::ConditionalBlock { ins };
            label->linkedBlock = condBlock;
            activeBlockList->push_back(condBlock);

            // Set current block list to the condition inner
            activeBlockList = &condBlock->inner;
            activeCodeBlock = nullptr;
         } else if (label->type == Label::ConditionalElse) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::LoopStart);
            assert(label->restoreBlockList);

            // Get the matching ConditionalBlock from the ConditionalStart label
            auto condBlock = reinterpret_cast<shadir::ConditionalBlock *>(label->linkedLabel->linkedBlock);

            // Set current block list to the condition else
            activeBlockList = &condBlock->innerElse;
            activeCodeBlock = nullptr;
         } else if (label->type == Label::ConditionalEnd) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::ConditionalStart);
            assert(label->restoreBlockList);

            // Get the matching ConditionalBlock from the ConditionalStart label
            auto condBlock = reinterpret_cast<shadir::ConditionalBlock *>(label->linkedLabel->linkedBlock);

            // Restore the previous block list
            activeBlockList = label->restoreBlockList;
            activeCodeBlock = nullptr;

            // Do not skip the current instruction, add it to a code block
            insertToCode = true;
         }

         // Start comparing to next label!
         ++labelItr;

         if (labelItr != labels.end()) {
            label = *labelItr;
         } else {
            label = nullptr;
         }
      }

      if (insertToCode) {
         assert(activeBlockList);

         if (!activeCodeBlock) {
            // Create a new block for active list
            activeCodeBlock = new shadir::CodeBlock {};
            activeBlockList->push_back(activeCodeBlock);
         }

         activeCodeBlock->code.push_back(ins);
      }
   }

   return true;
}

} // namespace latte
