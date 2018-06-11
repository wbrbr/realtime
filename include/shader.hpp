#ifndef SHADER_HPP
#define SHADER_HPP
#include <string>
#include <unordered_map>

unsigned int loadShaderProgramFromSource(const char* vSource, const char* fSource);
unsigned int loadShaderProgram(std::string vPath, std::string fPath);

class Shader
{
public:
    Shader(std::string vPath, std::string fPath);
    ~Shader();
    int getLoc(std::string name);
    unsigned int id();

private:
    unsigned int m_id;
    std::unordered_map<std::string, int> m_locs;
    std::string m_name;
};
#endif
