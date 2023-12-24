#include "GLUtil.h"

#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <Platform/gl.h>

namespace
{
std::pair<int, int> getErrorStringNumber(const std::string& errorLog)
{
    static const auto r = std::regex("\\d:(\\d+)\\((\\d+\\)).*");
    std::smatch match;
    if (std::regex_search(errorLog, match, r)) {
        assert(match.size() == 3);
        int lineNum{};
        std::istringstream(match[1].str()) >> lineNum;
        int charNum{};
        std::istringstream(match[2].str()) >> charNum;
        return {lineNum, charNum};
    }
    return {-1, -1};
}
}

namespace util
{
bool printShaderCompilationErrors(std::uint32_t shaderObject, const std::string& shaderSource)
{
    GLint status;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
        return true;
    }

    std::cerr << "Failed to compile shader: ";

    GLint logLength;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(logLength + 1, '\0');
    glGetShaderInfoLog(shaderObject, logLength, NULL, &log[0]);

    std::vector<std::string> lines;
    {
        std::stringstream ss(shaderSource);
        std::string line;
        while (std::getline(ss, line, '\n')) {
            lines.push_back(line);
        }
    }
    int lineNum = 1;
    std::stringstream ss(log);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        auto [errorLineNum, charNum] = getErrorStringNumber(line);
        if (errorLineNum != -1) {
            std::cerr << line << std::endl;
            std::cerr << "> " << lines[errorLineNum - 1] << std::endl;
            for (int i = 0; i < charNum + 4; ++i) {
                std::cerr << " ";
            }
            std::cerr << "^~\n";
            break;
        } else {
            std::cerr << line << std::endl;
        }
    }
    return false;
}

bool printShaderLinkErrors(std::uint32_t shaderProgram)
{
    GLint status;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) {
        return true;
    }

    std::string msg("Program linking failure: ");
    std::cerr << "Failed to link program: ";

    GLint logLength;
    glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
    std::cout << logLength << std::endl;
    std::string log(logLength + 1, '\0');
    glGetProgramInfoLog(shaderProgram, logLength, NULL, &log[0]);
    std::cerr << log << std::endl;
    return false;
}

}
