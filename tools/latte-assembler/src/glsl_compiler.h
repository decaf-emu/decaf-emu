#pragma once
#include "shader.h"
#include <string>

std::string
compileShader(std::string shaderAnalyzerPath,
              std::string shaderPath,
              ShaderType shaderType);
