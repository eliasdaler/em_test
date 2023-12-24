#pragma once

#include <cstdint>
#include <string>

namespace util
{
bool printShaderCompilationErrors(std::uint32_t shaderObject, const std::string& shaderSource);
bool printShaderLinkErrors(std::uint32_t shaderProgram);
}
