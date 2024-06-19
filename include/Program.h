#pragma once

#include<iostream>
#include<vector>
#include<fstream>
#include "common/common.h"

#define instring const std::string&
#define INFOLOG_LEN 512

namespace Festa {
	struct Shader {
		int type; uint id;
		Shader() {
			id = 0; type = 0;
		}
		//TODO print source and light up the errors;
		Shader(int type, const char* source) :type(type) {
			init(type, source);
		}
		Shader(int type, instring source) :type(type) {
			init(type, source.c_str());
		}
		Shader(instring file) {
			load(file);
		}
		void release() {
			glDeleteShader(id);
			id = 0, type = 0;
		}
		~Shader() {
			release();
		}
		void load(const Path& file) {
			const std::string post = file.extension();
			type = GL_VERTEX_SHADER;
			if (post == "fs")type = GL_FRAGMENT_SHADER;
			else if (post == "gs")type = GL_GEOMETRY_SHADER;
			else if (post != "vs")LOGGER.error("Invalid shader type: " + file.toString());
			std::string source; readString(file, source);
			init(type, source.c_str(), file);
		}
		void load(int type, const Path& file) {
			std::string source; readString(file, source);
			init(type, source.c_str(), file);
		}
		void init(int t, const char* source, instring name = "shader") {
			type = t;
			id = glCreateShader(type);
			glShaderSource(id, 1, &source, NULL);
			glCompileShader(id);

			int  success;
			char infoLog[INFOLOG_LEN];
			glGetShaderiv(id, GL_COMPILE_STATUS, &success);

			if (!success)
			{
				glGetShaderInfoLog(id, INFOLOG_LEN, NULL, infoLog);
				LOGGER.error(std::string(name + " ShaderCompilationError: ") + std::string(infoLog));
			}
		}
	};

	class Program {
	public:
		static Program* activeProgram;
		Program() {
			id = 0;
		}
		Program(const std::vector<Shader*>& shaders) {
			init(shaders);
		}
		Program(const std::vector<uint>& shaders) {
			init(shaders);
		}
		Program(const Program& program) {
			id = program.id;
		}
		void release() {
			if (id) {
				glDeleteProgram(id);
				id = 0;
			}
		}
		~Program() {
			release();
		}
		void init(const std::vector<Shader*>& shaders) {
			id = glCreateProgram();
			for (uint i = 0; i < shaders.size();i++)attach(shaders[i]->id);
			link();
		}
		void init(const std::vector<uint>& shaders) {
			id = glCreateProgram();
			for (uint shader : shaders)attach(shader);
			link();
		}
		void transformFeedbackVaryings(const std::vector<const char*>& varyings, uint mode = GL_INTERLEAVED_ATTRIBS) {
			glTransformFeedbackVaryings(id, int(varyings.size()), &varyings[0], mode);
		}
		void attach(uint shader)const {
			glAttachShader(id, shader);
		}
		void generate() {
			id = glCreateProgram();
		}
		void link()const {
			glLinkProgram(id);
			int success;
			glGetProgramiv(id, GL_LINK_STATUS, &success);
			if (!success) {
				char infoLog[INFOLOG_LEN];
				glGetProgramInfoLog(id, INFOLOG_LEN, NULL, infoLog);
				LOGGER.error(std::string("ProgramLinkingError: ") + std::string(infoLog));
			}
		}
		void bind() {
			glUseProgram(id);
			activeProgram = this;
		}
		static void unbind() {
			glUseProgram(0);
			activeProgram = 0;
		}
		inline int location(instring name)const {
			return glGetUniformLocation(id, name.c_str());
		}
		void setBool(instring name, bool value) const
		{
			glUniform1i(location(name), (int)value);
		}
		void setInt(instring name, int value) const
		{
			glUniform1i(location(name), value);
		}
		void setFloat(instring name, float value) const
		{
			glUniform1f(location(name), value);
		}
		void setVec2(instring name, const vec2 value) const
		{
			glUniform2fv(location(name), 1, &value[0]);
		}
		void setVec2(instring name, float x, float y) const
		{
			glUniform2f(location(name), x, y);
		}
		// ------------------------------------------------------------------------
		void setVec3(const std::string& name, const glm::vec3& value) const
		{
			glUniform3fv(location(name), 1, &value[0]);
		}
		void setVec3(const std::string& name, float x, float y, float z) const
		{
			glUniform3f(location(name), x, y, z);
		}
		// ------------------------------------------------------------------------
		void setVec4(const std::string& name, const glm::vec4& value) const
		{
			glUniform4fv(location(name), 1, &value[0]);
		}
		void setVec4(const std::string& name, float x, float y, float z, float w) const
		{
			glUniform4f(location(name), x, y, z, w);
		}
		// ------------------------------------------------------------------------
		void setMat2(const std::string& name, const glm::mat2& mat) const
		{
			glUniformMatrix2fv(location(name), 1, GL_FALSE, &mat[0][0]);
		}
		// ------------------------------------------------------------------------
		void setMat3(const std::string& name, const glm::mat3& mat) const
		{
			glUniformMatrix3fv(location(name), 1, GL_FALSE, &mat[0][0]);
		}
		// ------------------------------------------------------------------------
		void setMat4(const std::string& name, const glm::mat4& mat) const
		{
			glUniformMatrix4fv(location(name), 1, GL_FALSE, &mat[0][0]);
		}


	private:
		uint id;
	};

	//translate->rotate

}
