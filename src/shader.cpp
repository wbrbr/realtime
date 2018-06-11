#include "shader.hpp"
#include "GL/gl3w.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>

unsigned int loadShaderFromSource(unsigned int type, const char* source)
{
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
    }

    return shader;
}

unsigned int loadShaderProgramFromSource(const char* vSource, const char* fSource)
{
    unsigned int program = glCreateProgram();
    GLuint vertex, fragment;
    vertex = loadShaderFromSource(GL_VERTEX_SHADER, vSource);
    fragment = loadShaderFromSource(GL_FRAGMENT_SHADER, fSource);

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
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

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

unsigned int loadShaderProgram(std::string vPath, std::string fPath)
{
    std::ifstream vertex_file(vPath);
    std::stringstream buf;
    buf << vertex_file.rdbuf();
    std::string vertex_source = buf.str();
    buf.str("");
    std::ifstream fragment_file(fPath);
    buf << fragment_file.rdbuf();
    std::string fragment_source = buf.str();

    unsigned int program = loadShaderProgramFromSource(vertex_source.c_str(), fragment_source.c_str());
    return program;
}

Shader::Shader(std::string vPath, std::string fPath)
{
    m_id = loadShaderProgram(vPath, fPath);
    m_name = vPath + "|" + fPath;
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
        if (loc == -1) std::cerr << m_name << ": Invalid or inactive uniform: " << name << std::endl;
        m_locs[name] = loc; // WARNING: will even cache -1
        return loc;
    } else {
        return it->second;
    }
}
