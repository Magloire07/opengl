#pragma once

#include <cstdint>

class GLShaderLanguage
{
protected:
	uint32_t m_Program{ 0 };

	static uint32_t CompileShader(uint32_t type, const char* buffer);
	bool VerifyProgram();

public:
	inline uint32_t GetProgram() { return m_Program; }

};

class GLShader : public GLShaderLanguage
{
private:
	// Un Vertex Shader est execute pour chaque sommet (vertex)
	uint32_t m_VertexShader{ 0 };
	// Un Geometry Shader est execute pour chaque primitive
	uint32_t m_GeometryShader{ 0 };
	// Un Fragment Shader est execute pour chaque "pixel"
	// lors de la rasterization/remplissage de la primitive
	uint32_t m_FragmentShader{ 0 };


public:
	bool LoadVertexShader(const char* filename);
	bool LoadGeometryShader(const char* filename);
	bool LoadFragmentShader(const char* filename);
	bool Create();
	void Destroy();
};

class GLComputeShader : public GLShaderLanguage
{
private:
	uint32_t m_ComputeShader{ 0 };

public:
	inline uint32_t GetProgram() { return m_Program; }

	bool LoadComputeShader(const char* filename);
	bool Create();
	void Destroy();
};