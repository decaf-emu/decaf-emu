#include <cassert>
#include <stack>
#include <spdlog/details/format.h>
#include "latte_decoder.h"
#include "utils/bitutils.h"

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
   shadir::BlockList *restoreBlockList = nullptr;
   Label *linkedLabel = nullptr;
   shadir::Block *linkedBlock = nullptr;
};

using LabelList = std::vector<std::unique_ptr<Label>>;


/**
 * Generates a list of labels for interesting instructions.
 *
 * Takes a first pass over the linear instruction set and compiles a set of labels
 * of the interesting instructions, it also tries to combine these labels into groups
 * so that a LoopStart is matched up with a LoopEnd, or a matching set of
 * ConditionalStart, ConditionalElse and ConditionalEnd.
 */
static bool
labelify(Shader &shader, LabelList &labels)
{
   std::stack<shadir::CfInstruction *> loopStarts;// LOOP_START*
   std::stack<shadir::CfInstruction *> pushes;    // PUSH
   std::stack<shadir::CfInstruction *> pops;      // POP*
   std::stack<shadir::AluInstruction *> predSets; // PRED_SET*

   // Iterate over the code and find matching code patterns to labelify
   for (auto itr = shader.linear.begin(); itr != shader.linear.end(); ++itr) {
      auto &ins = *itr;

      if (ins->type == shadir::Instruction::CF) {
         auto cfIns = reinterpret_cast<shadir::CfInstruction *>(ins);

         if (cfIns->id == SQ_CF_INST_LOOP_START
             || cfIns->id == SQ_CF_INST_LOOP_START_DX10
             || cfIns->id == SQ_CF_INST_LOOP_START_NO_AL) {
            // Found a loop start, find a matching loop_end in the correct stack
            loopStarts.push(cfIns);
         } else if (cfIns->id == SQ_CF_INST_LOOP_END) {
            assert(loopStarts.size());

            // Create a LoopStart label
            auto labelStart = new Label { Label::LoopStart, loopStarts.top() };
            loopStarts.pop();
            labels.emplace_back(labelStart);

            // Create a LoopEnd label
            auto labelEnd = new Label { Label::LoopEnd, ins };
            labels.emplace_back(labelEnd);

            // Link the start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == SQ_CF_INST_LOOP_CONTINUE || cfIns->id == SQ_CF_INST_LOOP_BREAK) {
            assert(loopStarts.size());
            assert(predSets.size());

            // Create a ConditionalStart label for the last PRED_SET
            auto labelStart = new Label { Label::ConditionalStart, predSets.top() };
            predSets.pop();
            labels.emplace_back(labelStart);

            // Create a ConditionalEnd label for after the BREAK/CONTINUE instruction
            auto labelEnd = new Label { Label::ConditionalEnd, *(itr + 1) };
            labels.emplace_back(labelEnd);

            // Link the start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == SQ_CF_INST_JUMP) {
            assert(predSets.size());
            assert(pushes.size());

            // Find the end of the jump
            auto jumpEnd = std::find_if(itr, shader.linear.end(),
                                        [cfIns](auto &ins) {
               return ins->cfPC == cfIns->addr;
            });

            assert(jumpEnd != shader.linear.end());

            // Check if this jump has an ELSE branch
            auto jumpElse = shader.linear.end();

            if ((*jumpEnd)->type == shadir::Instruction::CF) {
               auto jumpEndCfIns = reinterpret_cast<shadir::CfInstruction *>(*jumpEnd);

               if (jumpEndCfIns->id == SQ_CF_INST_ELSE) {
                  jumpElse = jumpEnd;

                  // Find the real end of the jump
                  jumpEnd = std::find_if(jumpElse, shader.linear.end(),
                                         [jumpEndCfIns](auto &ins) {
                     return ins->cfPC == jumpEndCfIns->addr;
                  });

                  assert(jumpEnd != shader.linear.end());
               }
            }

            // Create a conditional start label
            auto labelStart = new Label { Label::ConditionalStart, predSets.top() };
            predSets.pop();
            labels.emplace_back(labelStart);

            // Create a conditional else label, if needed
            if (jumpElse != shader.linear.end()) {
               auto labelElse = new Label { Label::ConditionalElse, *jumpElse };
               labelElse->linkedLabel = labelStart;
               labels.emplace_back(labelElse);
            }

            // Create a conditional end label
            auto labelEnd = new Label { Label::ConditionalEnd, *jumpEnd };
            labels.emplace_back(labelEnd);

            // Eliminate our JUMP instruction
            labels.emplace_back(new Label { Label::Eliminated, ins });

            // Link start and end labels
            labelEnd->linkedLabel = labelStart;
            labelStart->linkedLabel = labelEnd;
         } else if (cfIns->id == SQ_CF_INST_PUSH) {
            pushes.push(cfIns);
         } else if (cfIns->id == SQ_CF_INST_POP) {
            pops.push(cfIns);
         }
      } else if (ins->type == shadir::Instruction::ALU) {
         auto aluIns = reinterpret_cast<shadir::AluInstruction *>(ins);

         if (aluIns->encoding == SQ_ALU_OP2) {
            auto flags = getInstructionFlags(aluIns->op2);

            if (flags & SQ_ALU_FLAG_PRED_SET) {
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
             [](auto &lhs, auto &rhs) {
      if (lhs->first->cfPC == rhs->first->cfPC) {
         assert(lhs->linkedLabel && rhs->linkedLabel);
         auto lhsLinkedPC = lhs->linkedLabel->first->cfPC;
         auto rhsLinkedPC = rhs->linkedLabel->first->cfPC;

         if (lhsLinkedPC < lhs->first->cfPC) {
            if (rhsLinkedPC < rhs->first->cfPC) {
               return lhsLinkedPC > rhsLinkedPC;
            } else {
               return true;
            }
         } else {
            return false;
         }
      }

      return lhs->first->cfPC < rhs->first->cfPC;
   });

   return true;
}


/**
 * Create code blocks from a set of linear instructions and labels.
 *
 * Iterates through the linear instructions and matches them up with labels,
 * we can use the labels to start building up blocks, i.e. when encountering
 * a new LoopStart label we create a new loop block, and insert code into that block
 * until we reach the matching LoopEnd label.
 */
static bool
blockify(Shader &shader, const LabelList &labels)
{
   shadir::CodeBlock *activeCodeBlock = nullptr;
   auto activeBlockList = &shader.blocks;
   auto labelItr = labels.begin();
   Label *label = nullptr;

   if (labelItr != labels.end()) {
      label = labelItr->get();
   }

   // Iterate over code and find matching labels to generate code blocks
   for (auto &ins : shader.linear) {
      bool insertToCode = true;

      while (label && label->first == ins) {
         // Most labels will skip current instruction
         insertToCode = false;

         if (label->type == Label::LoopStart) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::LoopEnd);

            // Save the active block list to the LoopEnd label
            label->linkedLabel->restoreBlockList = activeBlockList;

            // Create a new loop block
            auto loopBlock = new shadir::LoopBlock {};
            label->linkedBlock = loopBlock;
            activeBlockList->emplace_back(loopBlock);

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
            activeBlockList->emplace_back(condBlock);

            // Set current block list to the condition inner
            activeBlockList = &condBlock->inner;
            activeCodeBlock = nullptr;
         } else if (label->type == Label::ConditionalElse) {
            assert(label->linkedLabel);
            assert(label->linkedLabel->type == Label::ConditionalStart);

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
            label = labelItr->get();
         } else {
            label = nullptr;
         }
      }

      if (insertToCode) {
         assert(activeBlockList);

         if (!activeCodeBlock) {
            // Create a new block for active list
            activeCodeBlock = new shadir::CodeBlock {};
            activeBlockList->emplace_back(activeCodeBlock);
         }

         activeCodeBlock->code.push_back(ins);
      }
   }

   return true;
}


/**
 * Takes the linear set of instructions and convert to nice code blocks for easy code generation.
 */
bool
blockify(Shader &shader)
{
   LabelList labels;

   if (!linearify(shader)) {
      return false;
   }

   if (!labelify(shader, labels)) {
      return false;
   }

   return blockify(shader, labels);
}


/**
 * Recursive function to dump a list of blocks to string.
 */
static void
dumpBlockList(shadir::BlockList &blocks, fmt::MemoryWriter &out, std::string indent)
{
   for (auto &block : blocks) {
      switch (block->type) {
      case shadir::Block::CodeBlock:
      {
         auto codeBlock = reinterpret_cast<shadir::CodeBlock *>(block.get());

         for (auto &ins : codeBlock->code) {
            out << indent << ins->cfPC << " " << ins->name << "\n";
         }

         break;
      }
      case shadir::Block::Loop:
      {
         auto loopBlock = reinterpret_cast<shadir::LoopBlock *>(block.get());

         out << indent << "while(true) { // START_LOOP\n";
         dumpBlockList(loopBlock->inner, out, indent + "  ");
         out << indent << "} // END_LOOP\n";
         break;
      }
      case shadir::Block::Conditional:
      {
         auto condBlock = reinterpret_cast<shadir::ConditionalBlock *>(block.get());

         out << indent << "if(" << condBlock->condition->cfPC << " " << condBlock->condition->name << ") {\n";
         dumpBlockList(condBlock->inner, out, indent + "  ");

         if (condBlock->innerElse.size()) {
            out << indent << "} else {\n";
            dumpBlockList(condBlock->innerElse, out, indent + "  ");
         }

         out << indent << "} // END_COND\n";
         break;
      }
      default:
         throw std::logic_error("Invalid block type");
      }
   }
}


/**
 * Debug function to dump the internal block storage to a string.
 */
void
debugDumpBlocks(Shader &shader, std::string &out)
{
   fmt::MemoryWriter writer;
   dumpBlockList(shader.blocks, writer, "");
   out = writer.str();
}

} // namespace latte
