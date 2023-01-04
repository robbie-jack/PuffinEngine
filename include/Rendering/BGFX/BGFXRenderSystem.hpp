#pragma once

#include "ECS/System.hpp"

#include "Rendering/BGFX/BGFXVertex.hpp"

#include <vector>

namespace Puffin::Rendering::BGFX
{
	static VertexPC32 s_cubeVertices[] =
	{
		{{-1.0f,  1.0f,  1.0f}, 0xff000000 },
		{ {1.0f,  1.0f,  1.0f}, 0xff0000ff },
		{{-1.0f, -1.0f,  1.0f}, 0xff00ff00 },
		{ {1.0f, -1.0f,  1.0f}, 0xff00ffff },
		{{-1.0f,  1.0f, -1.0f}, 0xffff0000 },
		{ {1.0f,  1.0f, -1.0f}, 0xffff00ff },
		{{-1.0f, -1.0f, -1.0f}, 0xffffff00 },
		{ {1.0f, -1.0f, -1.0f}, 0xffffffff },
	};

	static const uint16_t s_cubeTriList[] =
	{
		0, 1, 2, // 0
		1, 3, 2,
		4, 6, 5, // 2
		5, 6, 7,
		0, 2, 4, // 4
		4, 2, 6,
		1, 5, 3, // 6
		5, 7, 3,
		0, 4, 1, // 8
		4, 5, 1,
		2, 3, 6, // 10
		6, 3, 7,
	};

	static bgfx::ShaderHandle LoadShader(const char* filename)
	{
		/*const char* shaderPath = "???";

		switch (bgfx::getRendererType())
		{
			case bgfx::RendererType::Noop:
			case bgfx::RendererType::Direct3D9:  shaderPath = "shaders/dx9/";   break;
			case bgfx::RendererType::Direct3D11: shaderPath = "shaders/dx11/";	break;
			case bgfx::RendererType::Direct3D12: shaderPath = "shaders/dx11/";  break;
			case bgfx::RendererType::Gnm:        shaderPath = "shaders/pssl/";  break;
			case bgfx::RendererType::Metal:      shaderPath = "shaders/metal/"; break;
			case bgfx::RendererType::OpenGL:     shaderPath = "shaders/glsl/";  break;
			case bgfx::RendererType::OpenGLES:   shaderPath = "shaders/essl/";  break;
			case bgfx::RendererType::Vulkan:     shaderPath = "shaders/spirv/"; break;
		}

		size_t shaderLen = strlen(shaderPath);
		size_t fileLen = strlen(filename);
		char* filePath = (char*)malloc(shaderLen + fileLen);
		memcpy(filePath, shaderPath, shaderLen);
		memcpy(&filePath[shaderLen], filename, fileLen);*/

		FILE* file = fopen(filename, "rb");
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		const bgfx::Memory* mem = bgfx::alloc(fileSize + 1);
		fread(mem->data, 1, fileSize, file);
		mem->data[mem->size - 1] = '\0';
		fclose(file);

		return bgfx::createShader(mem);
	}

	class BGFXRenderSystem : public ECS::System
	{
	public:

		BGFXRenderSystem()
		{
			m_systemInfo.name = "BGFXRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		~BGFXRenderSystem() override {}

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override;
		void Stop() override {}
		void Cleanup() override;

	private:

		uint32_t m_frameCounter = 0;

		bgfx::VertexBufferHandle m_vbh;
		bgfx::IndexBufferHandle m_ibh;

		bgfx::ShaderHandle m_vsh, m_fsh;
		bgfx::ProgramHandle m_program;

		void UpdateComponents();
		void Draw();

	};
}