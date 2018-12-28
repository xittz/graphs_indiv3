#include "Shader.h"
#include <stdexcept>
#include <fstream>
#include <string>
#include <cassert>
#include <sstream>

using namespace helpers;

Shader::Shader(const std::string& shaderCode, GLenum shaderType) : _shaderId(0), _refCount(NULL) {
    _shaderId = glCreateShader(shaderType);
    if(_shaderId == 0)
        throw std::runtime_error("glCreateShader failed");
    
    const char* code = shaderCode.c_str();
    glShaderSource(_shaderId, 1, (const GLchar**)&code, NULL);
    glCompileShader(_shaderId);
    
    GLint status;
    glGetShaderiv(_shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::string msg("Compile failure in shader:\n");
        
        GLint infoLogLength;
        glGetShaderiv(_shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* strInfoLog = new char[infoLogLength + 1];
        glGetShaderInfoLog(_shaderId, infoLogLength, NULL, strInfoLog);
        msg += strInfoLog;
        delete[] strInfoLog;
        
        glDeleteShader(_shaderId); _shaderId = 0;
        throw std::runtime_error(msg);
    }
    
    _refCount = new unsigned;
    *_refCount = 1;
}

Shader::Shader(const Shader& other) :
    _shaderId(other._shaderId),
    _refCount(other._refCount)
{
    _retain();
}

Shader::~Shader() {
    if(_refCount) _release();
}

GLuint Shader::shaderId() const {
    return _shaderId;
}

Shader& Shader::operator = (const Shader& other) {
    _release();
    _shaderId = other._shaderId;
    _refCount = other._refCount;
    _retain();
    return *this;
}

Shader Shader::shaderFromFile(const std::string& filePath, GLenum shaderType) {
    std::ifstream f;
    f.open(filePath.c_str(), std::ios::in | std::ios::binary);
    if(!f.is_open()){
        throw std::runtime_error(std::string("Failed to open file: ") + filePath);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();
    Shader shader(buffer.str(), shaderType);
    return shader;
}

void Shader::_retain() {
    assert(_refCount);
    *_refCount += 1;
}

void Shader::_release() {
    assert(_refCount && *_refCount > 0);
    *_refCount -= 1;
    if(*_refCount == 0){
        glDeleteShader(_shaderId); _shaderId = 0;
        delete _refCount; _refCount = NULL;
    }
}

