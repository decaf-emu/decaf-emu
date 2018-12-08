#include "shader_compiler.h"
#include "gfd_comment_parser.h"
#include <excmd.h>
#include <fstream>
#include <spdlog/spdlog.h>

static bool
readFile(const std::string &path,
         std::vector<char> &buff)
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
compileFile(Shader &shader,
            const std::string &path)
{
   std::vector<char> src;

   if (!readFile(path, src)) {
      return false;
   }

   return compileShaderCode(shader, src);
}

int main(int argc, char **argv)
{
   excmd::parser parser;
   excmd::option_state options;

   // Setup command line options
   parser.global_options()
      .add_option("h,help", excmd::description { "Show the help." })
      .add_option("vsh",
                  excmd::description { "Vertex shader to compile to gsh." },
                  excmd::value<std::string> {})
      .add_option("psh",
                  excmd::description { "Pixel shader to compile to gsh." },
                  excmd::value<std::string> {})
      .add_option("align",
                  excmd::description { "align data." });

   parser.add_command("help")
      .add_argument("command", excmd::value<std::string> { });

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

   Shader shader;

   try {
      if (options.has("compile")) {
         auto dst = options.get<std::string>("gsh");
         gfd::GFDFile gfd;

         if (options.has("vsh")) {
            auto src = options.get<std::string>("vsh");
            Shader shader;
            shader.path = src;
            shader.type = ShaderType::VertexShader;

            if (!compileFile(shader, src)) {
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

            if (!compileFile(shader, src)) {
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
