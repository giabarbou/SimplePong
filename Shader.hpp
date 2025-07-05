#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <GL/glew.h>
#include <vector>
#include <unordered_map>

struct Shader
{
	int id{};
	GLenum type{};
	std::string code;
};

class ShaderHandler
{
public:
	ShaderHandler();

	void add(Shader& shader);
	void compileShaders();
	void linkShaders();
	void validateShaders();
	GLint getUniformVariableId(const std::string &name);
	void enableShaders();
	void disableShaders();

private:

	std::unordered_map<std::string, int> uniform_vars;
	std::vector<Shader> shaders;
	int current_id = 0;

	bool program_created = false;
	bool compiled = false;
	bool linked = false;
	bool validated = false;
};

#endif