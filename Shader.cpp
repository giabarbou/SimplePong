#include "Shader.hpp"
#include <GL/glew.h>

static bool compileShader(Shader &shader)
{
    int id = shader.id;
    GLenum type = shader.type;
    std::string code = shader.code;

    const GLchar* c_code = code.c_str();
    GLint size = (GLint)code.size();

    glShaderSource(id, 1, &c_code, &size);
    glCompileShader(id);

    GLint result = 0;
    GLchar log[1024] = { 0 };

    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (!result) {
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        printf("Error compiling the %d shader: '%s'\n", type, log);
        return false;
    }

    return true;
}

ShaderHandler::ShaderHandler() {}

void ShaderHandler::add(Shader& shader)
{
    if (!program_created) {
        current_id = glCreateProgram();
        program_created = true;
    }

    shader.id = glCreateShader(shader.type);
    shaders.push_back(shader);
}

void ShaderHandler::compileShaders()
{
    for (Shader& shader : shaders) {
        compileShader(shader);
        glAttachShader(current_id, shader.id);
    }

    compiled = true;
}

void ShaderHandler::linkShaders()
{
    if (!compiled) return;

    glLinkProgram(current_id);

    GLint result = 0;
    GLchar log[1024] = { 0 };

    glGetProgramiv(current_id, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(current_id, sizeof(log), nullptr, log);
        printf("Error linking program: '%s'", log);
        return;
    }

    linked = true;
}

void ShaderHandler::validateShaders()
{
    if (!linked) return;

    glValidateProgram(current_id);

    GLint result = 0;
    GLchar log[1024] = { 0 };

    glGetProgramiv(current_id, GL_VALIDATE_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(current_id, sizeof(log), nullptr, log);
        printf("Error validating program: '%s'\n", log);
        return;
    }

    validated = true;
}

GLint ShaderHandler::getUniformVariableId(const std::string& name)
{
    return glGetUniformLocation(current_id, name.c_str());
}

void ShaderHandler::enableShaders()
{
    glUseProgram(current_id);
}

void ShaderHandler::disableShaders()
{
    glUseProgram(0);
}
