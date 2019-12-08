#include "shader_assembler.h"
#include "gfd_comment_parser.h"
#include "glsl_compiler.h"

#include <excmd.h>
#include <fmt/core.h>
#include <fstream>
#include <glslang/Public/ShaderLang.h>
#include <memory>
#include <optional>
#include <string>
#include <spdlog/spdlog.h>

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

static bool
assembleFile(Shader &shader,
             const std::string &path)
{
   std::string src;

   if (!readFile(path, src)) {
      return false;
   }

   return assembleShaderCode(shader, src);
}

int main(int argc, char **argv)
{
   excmd::parser parser;
   excmd::option_state options;

   // Setup command line options
   parser.global_options()
      .add_option("h,help", excmd::description { "Show the help." })
      .add_option("vsh",
                  excmd::description { "Vertex shader input." },
                  excmd::value<std::string> {})
      .add_option("psh",
                  excmd::description { "Pixel shader input." },
                  excmd::value<std::string> {})
      .add_option("align",
                  excmd::description { "Align data in gsh file." })
      .add_option("amd-shader-analyzer",
                  excmd::description{ "Path to AMD Shader Analyzer exe." },
                  excmd::value<std::string> {});

   parser.add_command("help")
      .add_argument("command", excmd::value<std::string> { });

   parser.add_command("assemble")
      .add_argument("gsh", excmd::value<std::string> { });

   parser.add_command("compile")
      .add_argument("gsh", excmd::value<std::string> { });

   // Parse command line
   try {
      options = parser.parse(argc, argv);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing command line: " << ex.what() << std::endl;
      return -1;
   }

   // Print help
   if (argc == 1 || options.has("help")) {
      if (options.has("command")) {
         std::cout << parser.format_help("gfdtool", options.get<std::string>("command")) << std::endl;
      } else {
         std::cout << parser.format_help("gfdtool") << std::endl;
      }

      return 0;
   }


   try {
      if (options.has("compile")) {
         if (!options.has("amd-shader-analyzer")) {
            std::cout << "Compile requires amd-shader-analyzer" << std::endl;
            return -1;
         }

         if (!options.has("vsh") || !options.has("psh")) {
            std::cout << "Compile requires vsh and psh" << std::endl;
            return -1;
         }

         auto vertexShaderPath = options.get<std::string>("vsh");
         auto pixelShaderPath = options.get<std::string>("psh");
         auto shaderAnalyzerPath = options.get<std::string>("amd-shader-analyzer");
         auto outGshPath = options.get<std::string>("gsh");
         glslang::InitializeProcess();

         auto vertexShaderAssembly = compileShader(shaderAnalyzerPath, vertexShaderPath, ShaderType::VertexShader);
         if (vertexShaderAssembly.empty()) {
            std::cout << "Failed to compile vertex shader " << vertexShaderPath << std::endl;
            return -1;
         }

         auto pixelShaderAssembly = compileShader(shaderAnalyzerPath, pixelShaderPath, ShaderType::PixelShader);
         if (pixelShaderAssembly.empty()) {
            std::cout << "Failed to compile pixel shader " << pixelShaderPath << std::endl;
            return -1;
         }

         auto vertexShader = Shader { };
         vertexShader.path = vertexShaderPath;
         vertexShader.type = ShaderType::VertexShader;
         if (!assembleShaderCode(vertexShader, vertexShaderAssembly)) {
            std::cout << "Failed to assemble vertex shader" << std::endl;
            return -1;
         }

         auto pixelShader = Shader { };
         pixelShader.path = pixelShaderPath;
         pixelShader.type = ShaderType::PixelShader;
         if (!assembleShaderCode(pixelShader, pixelShaderAssembly)) {
            std::cout << "Failed to assemble pixel shader" << std::endl;
            return -1;
         }

         auto gfd = gfd::GFDFile { };
         if (!gfdAddVertexShader(gfd, vertexShader)) {
            std::cout << "Failed to add vertex shader to gfd" << std::endl;
            return -1;
         }

         if (!gfdAddPixelShader(gfd, pixelShader)) {
            std::cout << "Failed to add pixel shader to gfd" << std::endl;
            return -1;
         }

         if (!gfd::writeFile(gfd, outGshPath, options.has("align"))) {
            std::cout << "Failed to add write gfd" << std::endl;
            return -1;
         }

         glslang::FinalizeProcess();
      } else if (options.has("assemble")) {
         auto dst = options.get<std::string>("gsh");
         gfd::GFDFile gfd;

         if (options.has("vsh")) {
            auto src = options.get<std::string>("vsh");
            Shader shader;
            shader.path = src;
            shader.type = ShaderType::VertexShader;

            if (!assembleFile(shader, src)) {
               return -1;
            }

            if (!gfdAddVertexShader(gfd, shader)) {
               return -1;
            }
         }

         if (options.has("psh")) {
            auto src = options.get<std::string>("psh");
            Shader shader;
            shader.path = src;
            shader.type = ShaderType::PixelShader;

            if (!assembleFile(shader, src)) {
               return -1;
            }

            if (!gfdAddPixelShader(gfd, shader)) {
               return -1;
            }
         }

         if (!gfd::writeFile(gfd, dst, options.has("align"))) {
            return -1;
         }
      } else {
         return -1;
      }
   } catch (parse_exception e) {
      std::cout << "Parse exception: " << e.what() << std::endl;
      return -1;
   } catch (gfd_header_parse_exception e) {
      std::cout << "GFD header parse exception: " << e.what() << std::endl;
      return -1;
   }

   return 0;
}
