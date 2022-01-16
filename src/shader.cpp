#include "shader.hpp"
#include "GL/gl3w.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

unsigned int loadShaderFromFile(unsigned int type, std::string path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening vertex shader: " << path << std::endl;
        exit(1);
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source_str = buf.str();

    const char* source = source_str.c_str();

    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);
        char* shader_log = static_cast<char*>(malloc(log_size));
        glGetShaderInfoLog(shader, log_size, NULL, shader_log);
        if (type == GL_VERTEX_SHADER) {
            std::cerr << "Vertex:" << shader_log << std::endl;
        } else {
            std::cerr << "Fragment:" << shader_log << std::endl;
        }
        exit(1);
    }

    return shader;
}

unsigned int createProgram(unsigned int* shaders, unsigned int num_shaders)
{
    unsigned int program = glCreateProgram();
    for (unsigned int i = 0; i < num_shaders; i++) {
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        char* program_log = (char*)malloc(log_length * sizeof(char));
        glGetProgramInfoLog(program, log_length, NULL, program_log);
        std::cerr << "Linking error: " << program_log << std::endl;
        free(program_log);
    }

    return program;
}


Shader::Shader(std::string vPath, std::string fPath)
{
    m_vertex_path = vPath;
    m_fragment_path = fPath;
    unsigned int vertex_shader = loadShaderFromFile(GL_VERTEX_SHADER, vPath);
    unsigned int fragment_shader = loadShaderFromFile(GL_FRAGMENT_SHADER, fPath);
    unsigned int shaders[2] = { vertex_shader, fragment_shader };
    m_id = createProgram(shaders, 2);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    m_name = vPath + "|" + fPath;
}

Shader::Shader(std::string cPath)
{
    m_compute_path = cPath;
    unsigned int shader = loadShaderFromFile(GL_COMPUTE_SHADER, cPath);
    m_id = createProgram(&shader, 1);
    glDeleteShader(shader);
    m_name = cPath;
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
}

unsigned int Shader::id()
{
    return m_id;
}

int Shader::getLoc(std::string name)
{
    auto it = m_locs.find(name);
    if (it == m_locs.end()) {
        int loc = glGetUniformLocation(m_id, name.c_str());
        if (loc == -1)
            std::cerr << m_name << ": Invalid or inactive uniform: " << name << std::endl;
        m_locs[name] = loc; // WARNING: will even cache -1
        return loc;
    } else {
        return it->second;
    }
}

void Shader::bind()
{
    glUseProgram(m_id);
}

void Shader::unbind()
{
    glUseProgram(0);
}

void Shader::reload()
{
    glDeleteProgram(m_id);
    // TODO: deduplicate code
    if (!m_vertex_path.empty()) {
        unsigned int vertex_shader = loadShaderFromFile(GL_VERTEX_SHADER, m_vertex_path);
        unsigned int fragment_shader = loadShaderFromFile(GL_FRAGMENT_SHADER, m_fragment_path);
        unsigned int shaders[2] = { vertex_shader, fragment_shader };
        m_id = createProgram(shaders, 2);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    } else {
        unsigned int shader = loadShaderFromFile(GL_COMPUTE_SHADER, m_compute_path);
        m_id = createProgram(&shader, 1);
        glDeleteShader(shader);
    }
    m_locs.clear();
}
