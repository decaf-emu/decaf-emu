#include "shader_assembler.h"

#include <common/align.h>
#include <common/platform.h>
#include <common/platform_winapi_string.h>
#include <fmt/format.h>
#include <fstream>
#include <glslang/Include/Types.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

static bool
readFile(const std::string &path,
         std::string &buff)
{
   std::ifstream ifs { path, std::ios::in | std::ios::binary };
   if (ifs.fail()) {
      return false;
   }

   buff.resize(static_cast<unsigned int>(ifs.seekg(0, std::ios::end).tellg()));
   if (!buff.empty()) {
      ifs.seekg(0, std::ios::beg).read(&buff[0], static_cast<std::streamsize>(buff.size()));
   }

   return true;
}

namespace glslang {
const TBuiltInResource DefaultTBuiltInResource = {
   /* .MaxLights = */ 32,
   /* .MaxClipPlanes = */ 6,
   /* .MaxTextureUnits = */ 32,
   /* .MaxTextureCoords = */ 32,
   /* .MaxVertexAttribs = */ 64,
   /* .MaxVertexUniformComponents = */ 4096,
   /* .MaxVaryingFloats = */ 64,
   /* .MaxVertexTextureImageUnits = */ 32,
   /* .MaxCombinedTextureImageUnits = */ 80,
   /* .MaxTextureImageUnits = */ 32,
   /* .MaxFragmentUniformComponents = */ 4096,
   /* .MaxDrawBuffers = */ 32,
   /* .MaxVertexUniformVectors = */ 128,
   /* .MaxVaryingVectors = */ 8,
   /* .MaxFragmentUniformVectors = */ 16,
   /* .MaxVertexOutputVectors = */ 16,
   /* .MaxFragmentInputVectors = */ 15,
   /* .MinProgramTexelOffset = */ -8,
   /* .MaxProgramTexelOffset = */ 7,
   /* .MaxClipDistances = */ 8,
   /* .MaxComputeWorkGroupCountX = */ 65535,
   /* .MaxComputeWorkGroupCountY = */ 65535,
   /* .MaxComputeWorkGroupCountZ = */ 65535,
   /* .MaxComputeWorkGroupSizeX = */ 1024,
   /* .MaxComputeWorkGroupSizeY = */ 1024,
   /* .MaxComputeWorkGroupSizeZ = */ 64,
   /* .MaxComputeUniformComponents = */ 1024,
   /* .MaxComputeTextureImageUnits = */ 16,
   /* .MaxComputeImageUniforms = */ 8,
   /* .MaxComputeAtomicCounters = */ 8,
   /* .MaxComputeAtomicCounterBuffers = */ 1,
   /* .MaxVaryingComponents = */ 60,
   /* .MaxVertexOutputComponents = */ 64,
   /* .MaxGeometryInputComponents = */ 64,
   /* .MaxGeometryOutputComponents = */ 128,
   /* .MaxFragmentInputComponents = */ 128,
   /* .MaxImageUnits = */ 8,
   /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
   /* .MaxCombinedShaderOutputResources = */ 8,
   /* .MaxImageSamples = */ 0,
   /* .MaxVertexImageUniforms = */ 0,
   /* .MaxTessControlImageUniforms = */ 0,
   /* .MaxTessEvaluationImageUniforms = */ 0,
   /* .MaxGeometryImageUniforms = */ 0,
   /* .MaxFragmentImageUniforms = */ 8,
   /* .MaxCombinedImageUniforms = */ 8,
   /* .MaxGeometryTextureImageUnits = */ 16,
   /* .MaxGeometryOutputVertices = */ 256,
   /* .MaxGeometryTotalOutputComponents = */ 1024,
   /* .MaxGeometryUniformComponents = */ 1024,
   /* .MaxGeometryVaryingComponents = */ 64,
   /* .MaxTessControlInputComponents = */ 128,
   /* .MaxTessControlOutputComponents = */ 128,
   /* .MaxTessControlTextureImageUnits = */ 16,
   /* .MaxTessControlUniformComponents = */ 1024,
   /* .MaxTessControlTotalOutputComponents = */ 4096,
   /* .MaxTessEvaluationInputComponents = */ 128,
   /* .MaxTessEvaluationOutputComponents = */ 128,
   /* .MaxTessEvaluationTextureImageUnits = */ 16,
   /* .MaxTessEvaluationUniformComponents = */ 1024,
   /* .MaxTessPatchComponents = */ 120,
   /* .MaxPatchVertices = */ 32,
   /* .MaxTessGenLevel = */ 64,
   /* .MaxViewports = */ 16,
   /* .MaxVertexAtomicCounters = */ 0,
   /* .MaxTessControlAtomicCounters = */ 0,
   /* .MaxTessEvaluationAtomicCounters = */ 0,
   /* .MaxGeometryAtomicCounters = */ 0,
   /* .MaxFragmentAtomicCounters = */ 8,
   /* .MaxCombinedAtomicCounters = */ 8,
   /* .MaxAtomicCounterBindings = */ 1,
   /* .MaxVertexAtomicCounterBuffers = */ 0,
   /* .MaxTessControlAtomicCounterBuffers = */ 0,
   /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
   /* .MaxGeometryAtomicCounterBuffers = */ 0,
   /* .MaxFragmentAtomicCounterBuffers = */ 1,
   /* .MaxCombinedAtomicCounterBuffers = */ 1,
   /* .MaxAtomicCounterBufferSize = */ 16384,
   /* .MaxTransformFeedbackBuffers = */ 4,
   /* .MaxTransformFeedbackInterleavedComponents = */ 64,
   /* .MaxCullDistances = */ 8,
   /* .MaxCombinedClipAndCullDistances = */ 8,
   /* .MaxSamples = */ 4,
   /* .maxMeshOutputVerticesNV = */ 256,
   /* .maxMeshOutputPrimitivesNV = */ 512,
   /* .maxMeshWorkGroupSizeX_NV = */ 32,
   /* .maxMeshWorkGroupSizeY_NV = */ 1,
   /* .maxMeshWorkGroupSizeZ_NV = */ 1,
   /* .maxTaskWorkGroupSizeX_NV = */ 32,
   /* .maxTaskWorkGroupSizeY_NV = */ 1,
   /* .maxTaskWorkGroupSizeZ_NV = */ 1,
   /* .maxMeshViewCountNV = */ 4,

   /* .limits = */ {
   /* .nonInductiveForLoops = */ 1,
   /* .whileLoops = */ 1,
   /* .doWhileLoops = */ 1,
   /* .generalUniformIndexing = */ 1,
   /* .generalAttributeMatrixVectorIndexing = */ 1,
   /* .generalVaryingIndexing = */ 1,
   /* .generalSamplerIndexing = */ 1,
   /* .generalVariableIndexing = */ 1,
   /* .generalConstantMatrixVectorIndexing = */ 1,
} };

} // namespace glslang

static int
getTypeBytes(const glslang::TType *type)
{
   auto size = 0;

   switch (type->getBasicType()) {
   case glslang::EbtInt8:
   case glslang::EbtUint8:
      size = 1;
      break;
   case glslang::EbtInt16:
   case glslang::EbtUint16:
   case glslang::EbtFloat16:
      size = 2;
      break;
   case glslang::EbtInt:
   case glslang::EbtUint:
   case glslang::EbtFloat:
      size = 4;
      break;
   case glslang::EbtInt64:
   case glslang::EbtUint64:
   case glslang::EbtDouble:
      size = 8;
      break;
   default:
      return 0;
   }

   if (type->isMatrix()) {
      size *= type->getMatrixCols();
      size *= type->getMatrixRows();
   }

   if (type->isArray()) {
      size *= type->getCumulativeArraySize();
   }

   if (type->isVector()) {
      size *= type->getVectorSize();
   }

   return size;
}

static std::string
getTypeString(const glslang::TType *type)
{
   if (type->isVector()) {
      auto result = std::string { };

      switch (type->getBasicType()) {
      case glslang::EbtFloat:
         break;
      case glslang::EbtBool:
         result = "b";
         break;
      case glslang::EbtInt:
         result = "i";
         break;
      case glslang::EbtUint:
         result = "u";
         break;
      case glslang::EbtDouble:
         result = "d";
         break;
      default:
         return {}; // Invalid
      }

      result += "vec";
      result += std::to_string(type->getVectorSize());
      return result;
   }

   if (type->isMatrix()) {
      auto result = std::string { };

      if (type->getBasicType() == glslang::EbtDouble) {
         result += "d";
      } else if (type->getBasicType() != glslang::EbtFloat) {
         return {};
      }

      auto cols = type->getMatrixCols();
      auto rows = type->getMatrixRows();
      if (cols < 2 || cols > 4 || rows < 2 || rows > 4) {
         return {};
      }

      result += "mat";
      result += std::to_string(cols);

      if (rows != cols) {
         result += "x";
         result += std::to_string(rows);
      }

      return result;
   }

   switch (type->getBasicType()) {
   case glslang::EbtVoid:
      return "void";
   case glslang::EbtBool:
      return "bool";
   case glslang::EbtInt:
      return "int";
   case glslang::EbtUint:
      return "uint";
   case glslang::EbtFloat:
      return "float";
   case glslang::EbtDouble:
      return "double";
   default:
      return {}; // Invalid
   }
}

static std::unique_ptr<glslang::TShader>
parseShader(EShLanguage stage, std::string path)
{
   auto source = std::string { };
   if (!readFile(path, source)) {
      std::cout << "Could not read " << path << std::endl;
      return nullptr;
   }

   /*std::string header = "#version 410\n#extension GL_ARB_separate_shader_objects : enable\n";
   source.insert(source.begin(), std::begin(header), std::end(header));
   source.push_back('\0');*/

   const char *texts[1] = { source.c_str() };
   const char *paths[1] = { path.c_str() };
   auto shader = std::make_unique<glslang::TShader>(stage);
   shader->setStringsWithLengthsAndNames(texts, nullptr, paths, 1);
   shader->setUniformLocationBase(0);
   shader->setEntryPoint("main");

   auto resources = glslang::DefaultTBuiltInResource;
   auto messages = EShMessages::EShMsgDefault;
   if (!shader->parse(&resources, 120, false, messages)) {
      std::cout << "glslang failed to parse shader" << std::endl;
      std::cout << shader->getInfoLog() << std::endl;
      std::cout << shader->getInfoDebugLog() << std::endl;
      return nullptr;
   }

   shader->getIntermediate()->addRequestedExtension("GL_ARB_separate_shader_objects");
   return shader;
}

static std::string
parseGlslFileToHeader(glslang::TShader &shader)
{
   auto messages = EShMessages::EShMsgDefault;
   auto program = glslang::TProgram { };
   program.addShader(&shader);

   if (!program.link(messages)) {
      std::cout << "glslang failed to link shader for shader" << std::endl;
      std::cout << program.getInfoLog() << std::endl;
      std::cout << program.getInfoDebugLog() << std::endl;
      return {};
   }

   if (!program.buildReflection(EShReflectionDefault | EShReflectionAllBlockVariables | EShReflectionIntermediateIO)) {
      std::cout << "glslang failed to build reflection for shader" << std::endl;
      std::cout << program.getInfoLog() << std::endl;
      std::cout << program.getInfoDebugLog() << std::endl;
      return {};
   }

   /*
   auto mapper = new glslang::TGlslIoMapper { };
   auto resolver = new glslang::TDefaultGlslIoResolver { *program.getIntermediate(EShLanguage::EShLangVertex) };
   if (!program.mapIO(resolver, mapper)) {
      std::cout << "glslang failed to map io" << std::endl;
      std::cout << vertexShader->getInfoLog() << std::endl;
      std::cout << vertexShader->getInfoDebugLog() << std::endl;
      return -1;
   }
      */

   // AMD ShaderAnalyzer puts the in / out / uniforms in alphabetical order
   std::vector<glslang::TObjectReflection> inputs;
   std::vector<glslang::TObjectReflection> outputs;
   std::vector<glslang::TObjectReflection> uniformVars;
   std::vector<glslang::TObjectReflection> uniformBlocks;

   for (auto i = 0; i < program.getNumPipeInputs(); ++i) {
      const auto &input = program.getPipeInput(i);
      inputs.insert(std::upper_bound(inputs.begin(), inputs.end(), input,
         [](const auto &lhs, const auto &rhs) {
            return lhs.name < rhs.name;
         }), input);
   }

   for (auto i = 0; i < program.getNumPipeOutputs(); ++i) {
      const auto &output = program.getPipeOutput(i);
      outputs.insert(std::upper_bound(outputs.begin(), outputs.end(), output,
         [](const auto &lhs, const auto &rhs) {
            return lhs.name < rhs.name;
         }), output);
   }

   for (auto i = 0; i < program.getNumUniformVariables(); ++i) {
      const auto &uniform = program.getUniform(i);
      uniformVars.insert(std::upper_bound(uniformVars.begin(), uniformVars.end(), uniform,
         [](const auto &lhs, const auto &rhs) {
            return lhs.name < rhs.name;
         }), uniform);
   }

   for (auto i = 0; i < program.getNumUniformBlocks(); ++i) {
      const auto &uniform = program.getUniformBlock(i);
      uniformBlocks.insert(std::upper_bound(uniformBlocks.begin(), uniformBlocks.end(), uniform,
         [](const auto &lhs, const auto &rhs) {
            return lhs.name < rhs.name;
         }), uniform);
   }

   // Generate assembly annotation
   auto outVsh = fmt::memory_buffer { };
   auto outPsh = fmt::memory_buffer{ };
   fmt::format_to(outVsh, "\n");
   fmt::format_to(outPsh, "\n");

   // Process inputs for fragment shader
   auto pixelInputCount = 0;
   for (auto i = 0u; i < inputs.size(); ++i) {
      if (inputs[i].stages & EShLanguageMask::EShLangFragmentMask) {
         ++pixelInputCount;
      }
   }

   if (pixelInputCount) {
      fmt::format_to(outPsh, "; $NUM_SPI_PS_INPUT_CNTL = {}\n", pixelInputCount);
      for (auto i = 0; i < pixelInputCount; ++i) {
         fmt::format_to(outPsh, "; $SPI_PS_INPUT_CNTL[{}].SEMANTIC = {}\n", i, i);
      }
   }

   // Process inputs for vertex shader
   for (auto i = 0u; i < inputs.size(); ++i) {
      const auto &input = inputs[i];
      auto type = input.getType();
      auto typeName = getTypeString(type);
      auto typeBytes = getTypeBytes(type);
      if (typeName.empty() || typeBytes == 0) {
         std::cout << "Invalid type for input " << input.name << std::endl;
         return {};
      }

      if (input.stages & EShLanguageMask::EShLangVertexMask) {
         fmt::format_to(outVsh, "; $ATTRIB_VARS[{}].name = \"{}\"\n", i, input.name);
         fmt::format_to(outVsh, "; $ATTRIB_VARS[{}].type = \"{}\"\n", i, typeName);
         fmt::format_to(outVsh, "; $ATTRIB_VARS[{}].location = {}\n", i, i);

         if (type->isArray()) {
            fmt::format_to(outVsh, "; $ATTRIB_VARS[{}].count = {}\n", i, type->getCumulativeArraySize());
         }

         fmt::format_to(outVsh, "\n");
      }
   }

   // Process uniform vars for both shaders
   auto uniformVarsOffset = 0;
   for (auto i = 0u; i < uniformVars.size(); ++i) {
      const auto &uniform = uniformVars[i];
      auto type = uniform.getType();
      auto typeName = getTypeString(type);
      auto typeBytes = getTypeBytes(type);
      if (typeName.empty() || typeBytes == 0) {
         std::cout << "Invalid type for uniform " << uniform.name << std::endl;
         return {};
      }

      auto out = fmt::memory_buffer { };
      fmt::format_to(out, "; $UNIFORM_VARS[{}].name = \"{}\"\n", i, uniform.name);
      fmt::format_to(out, "; $UNIFORM_VARS[{}].type = \"{}\"\n", i, typeName);
      fmt::format_to(out, "; $UNIFORM_VARS[{}].offset = {}\n", i, uniformVarsOffset / 4);
      uniformVarsOffset += align_up(typeBytes, 4);

      if (type->isArray()) {
         fmt::format_to(out, "; $UNIFORM_VARS[{}].count = {}\n", i, type->getCumulativeArraySize());
      }

      fmt::format_to(out, "\n");

      if (uniform.stages & EShLanguageMask::EShLangVertexMask) {
         outVsh.append(out.begin(), out.end());
      }

      if (uniform.stages & EShLanguageMask::EShLangFragmentMask) {
         outPsh.append(out.begin(), out.end());
      }
   }

   if (!uniformBlocks.empty()) {
      std::cout << "Unimplemented uniform blocks" << std::endl;
      return {};
   }

   // Process output for vertex shader
   auto vertexOutputCount = 0;
   for (auto i = 0u; i < outputs.size(); ++i) {
      const auto &output = outputs[i];
      if (output.stages & EShLanguageMask::EShLangVertexMask) {
         auto qualifier = output.getType()->getQualifier();
         if (qualifier.storage == glslang::EvqVaryingOut) {
            ++vertexOutputCount;
         } else if (qualifier.builtIn == glslang::EbvPointSize) {
            fmt::format_to(outVsh, "; $PA_CL_VS_OUT_CNTL.USE_VTX_POINT_SIZE = true\n");
         }
      }
   }

   if (vertexOutputCount) {
      fmt::format_to(outVsh, "; $NUM_SPI_VS_OUT_ID = {}\n", (vertexOutputCount + 3) / 4);
      for (auto i = 0; i < vertexOutputCount; ++i) {
         fmt::format_to(outVsh, "; $SPI_VS_OUT_ID[{}].SEMANTIC_{} = {}\n", i / 4, i % 4, i);
      }
   }

   outVsh.push_back('\0');
   outPsh.push_back('\0');

   if (shader.getStage() == EShLanguage::EShLangVertex) {
      return std::string { outVsh.data() };
   } else {
      return std::string { outPsh.data() };
   }
}

static std::string
runAmdShaderAnalyzer(std::string shaderAnalyzerPath,
                     std::string shaderPath,
                     ShaderType shaderType)
{
   std::string args;
   args += "\"" + shaderAnalyzerPath + "\"";
   args += " \"" + shaderPath + "\"";
   args += " -ASIC RV730";

   if (shaderType == ShaderType::VertexShader) {
      args += " -P glsl_vs";
   } else if (shaderType == ShaderType::PixelShader) {
      args += " -P glsl_fs";
   }

   args += " -I tmp.txt";

#ifdef PLATFORM_WINDOWS
   SECURITY_ATTRIBUTES security_attributes;
   security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
   security_attributes.bInheritHandle = TRUE;
   security_attributes.lpSecurityDescriptor = nullptr;

   STARTUPINFOA si;
   PROCESS_INFORMATION pi;
   DWORD exitCode;
   ZeroMemory(&si, sizeof(STARTUPINFOA));
   ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
   si.cb = sizeof(si);
   if (!CreateProcessA(nullptr, args.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
      std::cout << "Failed to create AMD ShaderAnalyzer process" << std::endl;
      return {};
   }
   CloseHandle(pi.hThread);

   WaitForSingleObject(pi.hProcess, INFINITE);
   if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
      exitCode = static_cast<DWORD>(-1);
   }

   if (exitCode != 0) {
      std::cout << "AMD ShaderAnalyzer returned " << exitCode << std::endl;
      return {};
   }
#else
   std::cout << "runAmdShaderAnalyzer unimplemented on this platform" << std::endl;
   std::cout << "Consider using wine ? ShaderAnalyzer is Windows only anyway :D" << std::endl;
   return {};
#endif

   std::string assembly;
   if (!readFile("tmp.txt", assembly)) {
      std::cout << "Could not read AMD ShaderAnalyzer output" << std::endl;
      return {};
   }

   if (std::strncmp(assembly.c_str(), "; --------  Disassembly --------------------", strlen("; --------  Disassembly --------------------"))) {
      std::cout << "Unexpected AMD ShaderAnalyzer output" << std::endl;
      return {};
   }

   return assembly;
}

std::string
compileShader(std::string shaderAnalyzerPath,
              std::string shaderPath,
              ShaderType shaderType)
{
   EShLanguage language;
   if (shaderType == ShaderType::VertexShader) {
      language = EShLangVertex;
   } else if (shaderType == ShaderType::PixelShader) {
      language = EShLangFragment;
   } else {
      std::cout << "Invalid shader type" << std::endl;
      return {};
   }

   auto shader = parseShader(language, shaderPath);
   if (!shader) {
      std::cout << "Failed to parse shader " << shaderPath << std::endl;
      return {};
   }

   auto shaderHeader = parseGlslFileToHeader(*shader);
   if (shaderHeader.empty()) {
      std::cout << "Failed to generate shader header for " << shaderPath << std::endl;
      return {};
   }

   auto shaderCode = runAmdShaderAnalyzer(shaderAnalyzerPath, shaderPath, shaderType);
   if (shaderCode.empty()) {
      std::cout << "Failed to generate shader code for " << shaderPath << std::endl;
      return {};
   }

   return shaderHeader + shaderCode;
}
