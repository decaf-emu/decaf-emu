#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <locale>
#include <map>
#include "bigendianview.h"
#include "codetests.h"
#include "elf.h"
#include "interpreter.h"
#include "log.h"
#include "memory.h"
#include "modules/coreinit/coreinit_dynload.h"

namespace TargetId {
enum {
   GPR,
   GPR0 = GPR,
   GPR31 = GPR + 31,

   FPR,
   FPR0 = FPR + 0,
   FPR31 = FPR + 31,

   CRF,
   CRF0 = CRF + 0,
   CRF7 = CRF + 7,

   XERSO,
   XEROV,
   XERCA,
   XERBC,

   Max
};
}
typedef uint32_t Target;

struct Value
{
   enum Type
   {
      Uint32,
      Float,
      Double,
      String
   };

   Type type;

   union
   {
      uint32_t uint32Value;
      float floatValue;
      double doubleValue;
   };

   std::string stringValue;
};

struct TestDataField
{
   TestDataField()
      : hasInput(false), hasOutput(false) { }

   void setInput(const Value& val) {
      hasInput = true;
      input = val;
   }

   void setOutput(const Value& val) {
      hasOutput = true;
      output = val;
   }

   bool hasInput;
   bool hasOutput;
   Value input;
   Value output;
};

struct TestData
{
   uint32_t offset;
   TestDataField fields[TargetId::Max];
};

struct TestFile
{
   std::map<std::string, TestData> tests;
   std::vector<char> code;
};

namespace fs = std::experimental::filesystem;

static bool
loadTestElf(const std::string &path, TestFile &tests)
{
   // Open file
   auto file = std::ifstream { path, std::ifstream::binary };
   auto buffer = std::vector<char> {};

   if (!file.is_open()) {
      return false;
   }

   // Get size
   file.seekg(0, std::istream::end);
   auto size = (unsigned int)file.tellg();
   file.seekg(0, std::istream::beg);

   // Read whole file
   buffer.resize(size);
   file.read(buffer.data(), size);
   assert(file.gcount() == size);
   file.close();

   auto in = BigEndianView { buffer.data(), size };
   auto header = elf::Header {};
   auto info = elf::FileInfo {};
   auto sections = std::vector<elf::Section> {};

   // Read header
   if (!elf::readHeader(in, header)) {
      xError() << "Failed to readHeader";
      return false;
   }

   // Read sections
   if (!elf::readSections(in, header, sections)) {
      xError() << "Failed to readRPLSections";
      return false;
   }

   // Find the code and symbol sections
   elf::Section *codeSection = nullptr;
   elf::Section *symbolSection = nullptr;

   for (auto &section : sections) {
      if (section.header.type == elf::SHT_PROGBITS && (section.header.flags & elf::SHF_EXECINSTR)) {
         codeSection = &section;
      }

      if (section.header.type == elf::SHT_SYMTAB) {
         symbolSection = &section;
      }
   }

   if (!codeSection) {
      xError() << "Could not find code section";
      return false;
   }

   if (!symbolSection) {
      xError() << "Could not find symbol section";
      return false;
   }

   // Find all test functions (symbols)
   auto symView = BigEndianView { symbolSection->data.data(), symbolSection->data.size() };
   auto symStrTab = sections[symbolSection->header.link].data.data();
   auto symCount = symbolSection->data.size() / sizeof(elf::Symbol);

   for (auto i = 0u; i < symCount; ++i) {
      elf::Symbol symbol;
      elf::readSymbol(symView, symbol);
      auto type = symbol.info & 0xf;
      auto name = symbol.name + symStrTab;

      if (type == elf::STT_SECTION) {
         continue;
      }

      if (symbol.value == 0 && symbol.name == 0 && symbol.shndx == 0) {
         continue;
      }

      tests.tests[name].offset = symbol.value;
   }

   tests.code = std::move(codeSection->data);
   return true;
}

static std::string &
ltrim(std::string &s)
{
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
   return s;
}

static std::string &
rtrim(std::string &s)
{
   s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
   return s;
}

static std::string &
trim(std::string &s)
{
   return ltrim(rtrim(s));
}

static bool
parseAssignment(const std::string &in, std::string &lhs, std::string &rhs)
{
   auto epos = in.find_first_of('=');

   if (epos == std::string::npos) {
      return false;
   }

   lhs = trim(in.substr(0, epos));
   rhs = trim(in.substr(epos + 1));
   return true;
}

static bool
decodeLHS(const std::string &in, Target &target)
{
   if (in.find("r") == 0) {
      target = TargetId::GPR0 + std::stoul(in.substr(1));
      return true;
   } else if (in.find("f") == 0) {
      target = TargetId::FPR0 + std::stoul(in.substr(1));
      return true;
   } else if (in.find("crf") == 0) {
      target = TargetId::CRF0 + std::stoul(in.substr(3));
      return true;
   } else if (in == "xer.so") {
      target = TargetId::XERSO;
      return true;
   } else if (in == "xer.ov") {
      target = TargetId::XEROV;
      return true;
   } else if (in == "xer.ca") {
      target = TargetId::XERCA;
      return true;
   } else if (in == "xer.bc") {
      target = TargetId::XERBC;
      return true;
   }

   return false;
}

static bool
decodeValue(const std::string &in, Target &target, Value &value)
{
   value.stringValue = in;

   if (in.find("0x") == 0) {
      value.type = Value::Uint32;
      value.uint32Value = std::stoul(in.substr(2), 0, 16);
      return true;
   } else if (in.find("f") == in.size() - 1) {
      value.type = Value::Float;
      value.floatValue = std::stof(in);
      return true;
   } else if (in.find(".") != std::string::npos) {
      value.type = Value::Double;
      value.doubleValue = std::stod(in);
      return true;
   } else if (in.find_first_not_of("0123456789") != std::string::npos) {
      if (target >= TargetId::CRF0 && target <= TargetId::CRF7) {
         value.type = Value::Uint32;
         value.uint32Value = 0;

         if (in.find("Positive") != std::string::npos) {
            value.uint32Value |= ConditionRegisterFlag::Positive;
         }

         if (in.find("Negative") != std::string::npos) {
            value.uint32Value |= ConditionRegisterFlag::Negative;
         }

         if (in.find("Zero") != std::string::npos) {
            value.uint32Value |= ConditionRegisterFlag::Zero;
         }

         if (in.find("SummaryOverflow") != std::string::npos) {
            value.uint32Value |= ConditionRegisterFlag::SummaryOverflow;
         }

         return true;
      }

      value.type = Value::String;
      return true;
   }

   // Assume its a number
   value.type = Value::Uint32;
   value.uint32Value = std::stoul(in);
   return true;
}

static bool
parseTestSource(const std::string &path, TestFile &tests)
{
   auto file = std::ifstream { path };
   std::string name, lhs, rhs;
   Target target;
   Value value;
   TestData *data = nullptr;

   for (std::string line; std::getline(file, line); ) {
      line = trim(line);

      if (!line.size()) {
         continue;
      }

      // Decode test name
      if (line.back() == ':') {
         name = line;
         name.erase(name.end() - 1);
         data = &tests.tests[name];
      }

      // Decode input
      if (line.find("# in ") == 0) {
         line.erase(0, std::strlen("# in "));

         if (!parseAssignment(line, lhs, rhs)) {
            return false;
         }

         if (!decodeLHS(lhs, target)) {
            return false;
         }

         if (!decodeValue(rhs, target, value)) {
            return false;
         }

         data->fields[target].setInput(value);
      }

      // Decode output
      if (line.find("# out ") == 0) {
         line.erase(0, std::strlen("# out "));

         if (!parseAssignment(line, lhs, rhs)) {
            return false;
         }

         if (!decodeLHS(lhs, target)) {
            return false;
         }

         if (!decodeValue(rhs, target, value)) {
            return false;
         }

         data->fields[target].setOutput(value);
      }
   }

   return true;
}

Value getStateValue(ThreadState& state, Target t)
{
   Value v;
   if (t >= TargetId::GPR0 && t <= TargetId::GPR31) {
      v.type = Value::Type::Uint32;
      v.uint32Value = state.gpr[t - TargetId::GPR0];
   } else if (t >= TargetId::FPR0 && t <= TargetId::FPR31) {
      v.type = Value::Type::Double;
      v.doubleValue = state.fpr[t - TargetId::FPR0].value;
   } else if (t >= TargetId::CRF0 && t <= TargetId::CRF7) {
      v.type = Value::Type::Uint32;
      v.uint32Value = getCRF(&state, t - TargetId::CRF0);
   } else if (t == TargetId::XERSO) {
      v.type = Value::Type::Uint32;
      v.uint32Value = state.xer.so;
   } else if (t == TargetId::XEROV) {
      v.type = Value::Type::Uint32;
      v.uint32Value = state.xer.ov;
   } else if (t == TargetId::XERCA) {
      v.type = Value::Type::Uint32;
      v.uint32Value = state.xer.ca;
   } else if (t == TargetId::XERBC) {
      v.type = Value::Type::Uint32;
      v.uint32Value = state.xer.byteCount;
   } else {
      assert(0);
   }
   return v;
}

void setStateValue(ThreadState& state, Target t, Value v)
{
   if (t >= TargetId::GPR0 && t <= TargetId::GPR31) {
      assert(v.type == Value::Type::Uint32);
      state.gpr[t - TargetId::GPR0] = v.uint32Value;
   } else if (t >= TargetId::FPR0 && t <= TargetId::FPR31) {
      assert(v.type == Value::Type::Double);
      state.fpr[t - TargetId::FPR0].value = v.doubleValue;
   } else if (t >= TargetId::CRF0 && t <= TargetId::CRF7) {
      assert(v.type == Value::Type::Uint32);
      setCRF(&state, t - TargetId::CRF0, v.uint32Value);
   } else if (t == TargetId::XERSO) {
      assert(v.type == Value::Type::Uint32);
      state.xer.so = v.uint32Value;
   } else if (t == TargetId::XEROV) {
      assert(v.type == Value::Type::Uint32);
      state.xer.ov = v.uint32Value;
   } else if (t == TargetId::XERCA) {
      assert(v.type == Value::Type::Uint32);
      state.xer.ca = v.uint32Value;
   } else if (t == TargetId::XERBC) {
      assert(v.type == Value::Type::Uint32);
      state.xer.byteCount = v.uint32Value;
   } else {
      assert(0);
   }
}

std::string getTargetName(Target t) {
   if (t >= TargetId::GPR0 && t <= TargetId::GPR31) {
      return "GPR[" + std::to_string(t - TargetId::GPR0) + "]";
   } else if (t >= TargetId::FPR0 && t <= TargetId::FPR31) {
      return "FPR[" + std::to_string(t - TargetId::FPR0) + "]";
   } else if (t >= TargetId::CRF0 && t <= TargetId::CRF7) {
      return "CRF[" + std::to_string(t - TargetId::CRF0) + "]";
   } else if (t == TargetId::XERSO) {
      return "XER.SO";
   } else if (t == TargetId::XEROV) {
      return "XER.OV";
   } else if (t == TargetId::XERCA) {
      return "XER.CA";
   } else if (t == TargetId::XERBC) {
      return "XER.BC";
   } else {
      assert(0);
      return "ERROR";
   }
};

bool checkField(const TestDataField& field, Target target, ThreadState& state, ThreadState& ostate) {
   Value nv = getStateValue(state, target);
   Value ov = getStateValue(ostate, target);
   assert(nv.type == ov.type);

   std::string fieldName = getTargetName(target);
   if (field.hasOutput) {
      assert(field.output.type == nv.type);

      if (nv.type == Value::Type::Uint32) {
         if (nv.uint32Value != field.output.uint32Value) {
            xLog() << "Expected " << fieldName << " to be " 
               << Log::hex(field.output.uint32Value) << " but got " << Log::hex(nv.uint32Value);
            return false;
         }
      } else if (nv.type == Value::Type::Double) {
         if (nv.doubleValue != field.output.doubleValue) {
            xLog() << "Expected " << fieldName << " to be " 
               << field.output.doubleValue << " but got " << nv.doubleValue;
            return false;
         }
      } else {
         assert(0);
      }
   } else {
      if (nv.type == Value::Type::Uint32) {
         if (nv.uint32Value != ov.uint32Value) {
            xLog() << "Expected " << fieldName << " to be unchanged but" 
               << Log::hex(nv.uint32Value) << " != " << Log::hex(ov.uint32Value);
            return false;
         }
      } else if (nv.type == Value::Type::Double) {
         if (nv.uint32Value != ov.uint32Value) {
            xLog() << "Expected " << fieldName << " to be unchanged but " 
               << nv.doubleValue << " != " << ov.doubleValue;
            return false;
         }
      } else {
         assert(0);
      }
   }
   return true;
}

bool executeCodeTest(ThreadState& state, uint32_t baseAddress, const TestData& test, bool withJit) {
   for (auto i = 0; i < TargetId::Max; ++i) {
      if (test.fields[i].hasInput) {
         setStateValue(state, i, test.fields[i].input);
      }
   }

   // Save the original state for comparison later
   ThreadState originalState = state;

   // Execute test
   if (withJit) {
      gInterpreter.setJitEnabled(true);
      gInterpreter.execute(&state, baseAddress + test.offset);
   } else {
      gInterpreter.setJitEnabled(false);
      gInterpreter.execute(&state, baseAddress + test.offset);
   }

   bool result = true;
   for (auto i = 0; i < TargetId::Max; ++i) {
      result &= checkField(test.fields[i], i, state, originalState);
   }
   return result;
}

bool
executeCodeTests(const std::string &assembler, const std::string &directory)
{
   uint32_t baseAddress;

   if (std::system((assembler + " --version > nul").c_str()) != 0) {
      xError() << "Could not find assembler " << assembler;
      return false;
   }

   if (!fs::exists(directory)) {
      xError() << "Could not find test directory " << directory;
      return false;
   }

   // Allocate some memory to write code to
   if (OSDynLoad_MemAlloc(4096, 4, &baseAddress) != 0 || !baseAddress) {
      xError() << "Could not allocate memory for test code";
      return false;
   }

   // Find all tests in directory
   for (auto itr = fs::directory_iterator { directory }; itr != fs::directory_iterator(); ++itr) {
      TestFile tests;
      auto path = itr->path().generic_string();

      // Pares the source file
      if (!parseTestSource(path, tests)) {
         xError() << "Failed parsing source file for " << path;
         continue;
      }

      // Create assembler command
      auto as = assembler;
      as += " -a32 -be -mpower7 -mregnames -R -o tmp.elf ";
      as += path;

      // Execute assembler
      auto result = std::system(as.c_str());

      if (result != 0) {
         xError() << "Error assembling test " << path;
         continue;
      }

      // Load the elf
      if (!loadTestElf("tmp.elf", tests)) {
         xError() << "Error loading assembled elf for " << path;
         continue;
      }

      // Load code into memory
      memcpy(gMemory.translate(baseAddress), tests.code.data(), tests.code.size());

      // Execute tests
      ThreadState state;
      for (auto &test : tests.tests) {
         auto result = true;

         // Run test with all state set to 0x00
         xLog() << "Running `" << test.first << "` with 0x00 and NO JIT";
         memset(&state, 0x00, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second, false);

         // Run test with all state set to 0xFF
         xLog() << "Running `" << test.first << "` with 0xFF and NO JIT";
         memset(&state, 0xFF, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second, false);

         // Run test with all state set to 0x00
         xLog() << "Running `" << test.first << "` with 0x00 and JIT";
         memset(&state, 0x00, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second, true);
     
         // Run test with all state set to 0xFF
         xLog() << "Running `" << test.first << "` with 0xFF and JIT";
         memset(&state, 0xFF, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second, true);

         // BUT WAS IT SUCCESS??
         if (!result) {
            xLog() << "FAILED " << test.first;
         } else {
            xLog() << "PASSED " << test.first;
         }
      }

      // Cleanup elf
      fs::remove("tmp.elf");
   }

   return true;
}
