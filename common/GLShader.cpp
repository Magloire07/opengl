
//#include "stdafx.h"
#include "../common/GLShader.h"
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>

bool ValidateShader(GLuint shader)
{
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = new char[1 + infoLen];

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			std::cout << "Error compiling shader:" << infoLog << std::endl;

			delete[] infoLog;
		}

		// on supprime le shader object car il est inutilisable
		glDeleteShader(shader);

		return false;
	}

	return true;
}

char* LoadFile(const char* filename)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	fin.seekg(0, std::ios::end);
	uint32_t length = (uint32_t)fin.tellg();
	fin.seekg(0, std::ios::beg);
	char* buffer = nullptr;
	buffer = new char[length + 1];
	buffer[length] = '\0';
	fin.read(buffer, length);
	fin.close();	// non obligatoire ici

	return buffer;
}

uint32_t GLShaderLanguage::CompileShader(uint32_t type, const char* buffer)
{
	uint32_t shaderObject;

	// 2. Creer le shader object
	shaderObject = glCreateShader(type);
	glShaderSource(shaderObject, 1, &buffer, nullptr);
	//  3. le compiler
	glCompileShader(shaderObject);
	
	return shaderObject;
}

bool GLShaderLanguage::VerifyProgram()
{
	int32_t linked = 0;
	int32_t infoLen = 0;
	// verification du statut du linkage
	glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = new char[infoLen + 1];

			glGetProgramInfoLog(m_Program, infoLen, NULL, infoLog);
			std::cout << "Erreur de lien du programme: " << infoLog << std::endl;

			delete[] infoLog;
		}

		glDeleteProgram(m_Program);

		return false;
	}

	return true;
}

// ---

bool GLShader::LoadVertexShader(const char* filename)
{
	// 1. Charger le fichier en memoire
	char* buffer = LoadFile(filename);

	m_VertexShader = CompileShader(GL_VERTEX_SHADER, buffer);
	
	// 4. Nettoyer
	delete[] buffer;

	// 5. verifier le status de la compilation
	return ValidateShader(m_VertexShader);
}

bool GLShader::LoadGeometryShader(const char* filename)
{
	// 1. Charger le fichier en memoire
	char* buffer = LoadFile(filename);

	m_GeometryShader = CompileShader(GL_GEOMETRY_SHADER, buffer);
	
	// 4. Nettoyer
	delete[] buffer;

	// 5. verifier le status de la compilation
	return ValidateShader(m_GeometryShader);
}

bool GLShader::LoadFragmentShader(const char* filename)
{
	// 1. Charger le fichier en memoire
	char* buffer = LoadFile(filename);

	m_FragmentShader = CompileShader(GL_FRAGMENT_SHADER, buffer);
	
	// 4. Nettoyer
	delete[] buffer;

	// 5. verifier le status de la compilation
	return ValidateShader(m_FragmentShader);
}

bool GLShader::Create()
{
	m_Program = glCreateProgram();
	if (m_VertexShader) glAttachShader(m_Program, m_VertexShader);
	if (m_GeometryShader) glAttachShader(m_Program, m_GeometryShader);
	if (m_FragmentShader) glAttachShader(m_Program, m_FragmentShader);
	glLinkProgram(m_Program);

	return VerifyProgram();	
}

void GLShader::Destroy()
{
	if (m_Program) {
		if (m_VertexShader) glDetachShader(m_Program, m_VertexShader);
		if (m_FragmentShader) glDetachShader(m_Program, m_FragmentShader);
		if (m_GeometryShader) glDetachShader(m_Program, m_GeometryShader);
	}
	if (m_GeometryShader) glDeleteShader(m_GeometryShader);
	if (m_VertexShader) glDeleteShader(m_VertexShader);
	if (m_FragmentShader) glDeleteShader(m_FragmentShader);
	if (m_Program) glDeleteProgram(m_Program);
}

// ---

bool GLComputeShader::LoadComputeShader(const char* filename)
{
	// 1. Charger le fichier en memoire
	char* buffer = LoadFile(filename);

	m_ComputeShader = CompileShader(GL_COMPUTE_SHADER, buffer);
	
	// 4. Nettoyer
	delete[] buffer;

	// 5. verifier le status de la compilation
	return ValidateShader(m_ComputeShader);
}

bool GLComputeShader::Create()
{
	m_Program = glCreateProgram();
	glAttachShader(m_Program, m_ComputeShader);
	glLinkProgram(m_Program);

	return VerifyProgram();
}

void GLComputeShader::Destroy()
{
	glDetachShader(m_Program, m_ComputeShader);
	glDeleteShader(m_ComputeShader);
	glDeleteProgram(m_Program);
}