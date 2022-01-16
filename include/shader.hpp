#ifndef SHADER_HPP
#define SHADER_HPP
#include <string>
#include <unordered_map>

unsigned int loadShaderProgramFromSource(const char* vSource, const char* fSource);
unsigned int loadShaderProgram(std::string vPath, std::string fPath);

class Shader {
public:
    Shader(std::string vPath, std::string fPath);
    Shader(std::string cPath);
    ~Shader();
    int getLoc(std::string name);
    unsigned int id();
    void bind();
    void unbind();
    void reload();

private:
    unsigned int m_id;
    std::unordered_map<std::string, int> m_locs;
    std::string m_name;

    std::string m_vertex_path, m_fragment_path, m_compute_path;
};

struct ShaderID {
    unsigned int id;
};
#endif
