#pragma once
#include "shader.h"
#include <cstdint>
#include <libgfd/gfd.h>
#include <vector>
#include <string>

class gfd_header_parse_exception : public std::runtime_error
{
public:
   gfd_header_parse_exception(const std::string &m) :
      std::runtime_error(m)
   {
   }

private:
   std::string mMessage;
};

struct CommentKeyValue
{
   bool isValue() const
   {
      return member.empty() && index.empty();
   }

   bool isObject() const
   {
      return !member.empty() && index.empty();
   }

   bool isArrayOfValues() const
   {
      return !index.empty() && member.empty();
   }

   bool isArrayOfObjects() const
   {
      return !member.empty() && !index.empty();
   }

   std::string obj;
   std::string index;
   std::string member;
   std::string value;
};

void
ensureArrayOfObjects(const CommentKeyValue &kv);

void
ensureArrayOfValues(const CommentKeyValue &kv);

void
ensureObject(const CommentKeyValue &kv);

void
ensureValue(const CommentKeyValue &kv);

bool
parseComment(const std::string &comment,
             CommentKeyValue &out);

bool
parseValueBool(const std::string &value);

uint32_t
parseValueNumber(const std::string &value);

float
parseValueFloat(const std::string &v);

// compiler_gfd
bool
gfdAddVertexShader(gfd::GFDFile &file,
                   Shader &shader);

bool
gfdAddPixelShader(gfd::GFDFile &file,
                  Shader &shader);

cafe::gx2::GX2ShaderVarType
parseShaderVarType(const std::string &v);

cafe::gx2::GX2SamplerVarType
parseSamplerVarType(const std::string &v);

cafe::gx2::GX2ShaderMode
parseShaderMode(const std::string &v);

void
parseUniformBlocks(std::vector<gfd::GFDUniformBlock> &UniformBlocks,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value);

void
parseUniformVars(std::vector<gfd::GFDUniformVar> &uniformVars,
                 uint32_t index,
                 const std::string &member,
                 const std::string &value);
void
parseInitialValues(std::vector<gfd::GFDUniformInitialValue> &initialValues,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value);
void
parseLoopVars(std::vector<gfd::GFDLoopVar> &loopVars,
              uint32_t index,
              const std::string &member,
              const std::string &value);

void
parseSamplerVars(std::vector<gfd::GFDSamplerVar> &samplerVars,
                 uint32_t index,
                 const std::string &member,
                 const std::string &value);

// compiler_gfd_vsh
bool
parseShaderComments(gfd::GFDVertexShader &shader,
                    std::vector<std::string> &comments);

// compiler_gfd_psh
bool
parseShaderComments(gfd::GFDPixelShader &shader,
                    std::vector<std::string> &comments);
