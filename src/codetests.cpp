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

struct Target
{
   enum Type
   {
      GPR,
      FPR,
      CRF,
      XERSO,
      XEROV,
      XERCA,
      XERBC
   };

   Type type;
   uint32_t id;
};

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

struct TestData
{
   uint32_t offset;
   std::vector<std::pair<Target, Value>> inputs;
   std::vector<std::pair<Target, Value>> outputs;
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
      target.type = Target::GPR;
      target.id = std::stoul(in.substr(1));
      return true;
   } else if (in.find("f") == 0) {
      target.type = Target::FPR;
      target.id = std::stoul(in.substr(1));
      return true;
   } else if (in.find("crf") == 0) {
      target.type = Target::CRF;
      target.id = std::stoul(in.substr(3));
      return true;
   } else if (in == "xer.so") {
      target.type = Target::XERSO;
      return true;
   } else if (in == "xer.ov") {
      target.type = Target::XEROV;
      return true;
   } else if (in == "xer.ca") {
      target.type = Target::XERCA;
      return true;
   } else if (in == "xer.bc") {
      target.type = Target::XERBC;
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
      value.uint32Value = std::stoul(in.substr(2));
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
      if (target.type == Target::CRF) {
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

         data->inputs.push_back(std::make_pair(target, value));
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

         data->outputs.push_back(std::make_pair(target, value));
      }
   }

   return true;
}

struct TestStateField {
   TestStateField() {
      hasInput = false;
      hasOutput = false;
   }

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

struct TestState {
   TestStateField gpr[32];
   TestStateField fpr[32];
   TestStateField crf[8];
   TestStateField xerso;
   TestStateField xerov;
   TestStateField xerca;
   TestStateField xerbc;
};

TestState testStateFromTest(const TestData& test) {
   TestState tState;

   // Setup tState
   for (auto &input : test.inputs) {
      auto &target = input.first;
      auto &value = input.second;

      switch (target.type) {
      case Target::GPR:
         tState.gpr[target.id].setInput(value);
         break;
      case Target::FPR:
         tState.fpr[target.id].setInput(value);
         break;
      case Target::CRF:
         tState.crf[target.id].setInput(value);
         break;
      case Target::XERSO:
         tState.xerso.setInput(value);
         break;
      case Target::XEROV:
         tState.xerov.setInput(value);
         break;
      case Target::XERCA:
         tState.xerca.setInput(value);
         break;
      case Target::XERBC:
         tState.xerbc.setInput(value);
         break;
      }
   }

   for (auto &input : test.outputs) {
      auto &target = input.first;
      auto &value = input.second;

      switch (target.type) {
      case Target::GPR:
         tState.gpr[target.id].setOutput(value);
         break;
      case Target::FPR:
         tState.fpr[target.id].setOutput(value);
         break;
      case Target::CRF:
         tState.crf[target.id].setOutput(value);
         break;
      case Target::XERSO:
         tState.xerso.setOutput(value);
         break;
      case Target::XEROV:
         tState.xerov.setOutput(value);
         break;
      case Target::XERCA:
         tState.xerca.setOutput(value);
         break;
      case Target::XERBC:
         tState.xerbc.setOutput(value);
         break;
      }
   }

   return tState;
}

bool checkIntField(const TestStateField& field, uint32_t nv, uint32_t ov, const std::string& name, int id = -1) {
   std::string fullName = name;
   if (id >= 0) {
      fullName += "[" + std::to_string(id) + "]";
   }

   if (field.hasOutput) {
      if (nv != field.output.uint32Value) {
         xLog() << "Expected " << fullName << " to be " << Log::hex(field.output.uint32Value) << " but got " << Log::hex(nv);
         return false;
      }
   } else {
      if (nv != ov) {
         xLog() << "Expected " << fullName << " to be unchanged (" << Log::hex(nv) << " != " << Log::hex(ov) << ")";
         return false;
      }
   }
   return true;
}

bool checkDoubleField(const TestStateField& field, double nv, double ov, const std::string& name, int id = -1) {
   std::string fullName = name;
   if (id >= 0) {
      fullName += "[" + std::to_string(id) + "]";
   }

   if (field.hasOutput) {
      if (nv != field.output.doubleValue) {
         xLog() << "Expected " << fullName << " to be " << field.output.doubleValue << " but got " << nv;
         return false;
      }
   } else {
      if (*(uint64_t*)(&nv) != *(uint64_t*)(&ov)) {
         xLog() << "Expected " << fullName << " to be unchanged (" << nv << " != " << ov << ")";
         return false;
      }
   }
   return true;
}

bool executeCodeTest(ThreadState& state, uint32_t baseAddress, const TestData& test) {
   TestState tState = testStateFromTest(test);

   for (auto i = 0; i < 32; ++i) {
      auto field = tState.gpr[i];
      if (field.hasInput) {
         state.gpr[i] = field.input.uint32Value;
      }
   }
   for (auto i = 0; i < 32; ++i) {
      auto field = tState.fpr[i];
      if (field.hasInput) {
         state.fpr[i].value = field.input.doubleValue;
      }
   }
   for (auto i = 0; i < 8; ++i) {
      auto field = tState.crf[i];
      if (field.hasInput) {
         setCRF(&state, i, field.input.uint32Value);
      }
   }
   if (tState.xerso.hasInput) {
      state.xer.so = tState.xerso.input.uint32Value;
   }
   if (tState.xerov.hasInput) {
      state.xer.ov = tState.xerov.input.uint32Value;
   }
   if (tState.xerca.hasInput) {
      state.xer.ca = tState.xerca.input.uint32Value;
   }
   if (tState.xerbc.hasInput) {
      state.xer.byteCount = tState.xerbc.input.uint32Value;
   }

   // Save the original state for comparison later
   ThreadState originalState = state;

   // Execute test
   gInterpreter.execute(&state, baseAddress + test.offset);

   bool result = true;

   for (auto i = 0; i < 32; ++i) {
      result &= checkIntField(tState.gpr[i], state.gpr[i], originalState.gpr[i], "GPR", i);
   }
   for (auto i = 0; i < 32; ++i) {
      result &= checkDoubleField(tState.fpr[i], state.fpr[i].value, originalState.fpr[i].value, "FPR", i);
   }
   for (auto i = 0; i < 8; ++i) {
      result &= checkIntField(tState.crf[i], getCRF(&state, i), getCRF(&originalState, i), "CRF", i);
   }
   result &= checkIntField(tState.xerso, state.xer.so, originalState.xer.so, "XER.SO");
   result &= checkIntField(tState.xerov, state.xer.ov, originalState.xer.ov, "XER.OV");
   result &= checkIntField(tState.xerca, state.xer.ca, originalState.xer.ca, "XER.CA");
   result &= checkIntField(tState.xerbc, state.xer.byteCount, originalState.xer.byteCount, "XER.BC");
  
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
         xLog() << "Running `" << test.first << "` with 0x00";
         memset(&state, 0x00, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second);
     
         // Run test with all state set to 0xFF
         xLog() << "Running `" << test.first << "` with 0xFF";
         memset(&state, 0xFF, sizeof(ThreadState));
         result &= executeCodeTest(state, baseAddress, test.second);

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
