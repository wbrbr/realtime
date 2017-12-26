#ifndef SHADER_HPP
#define SHADER_HPP
#include <string>

unsigned int loadShaderProgramFromSource(const char* vSource, const char* fSource);
unsigned int loadShaderProgram(std::string vPath, std::string fPath);
#endif
