#pragma once

#include <memory>

#include "ECS/System.hpp"

#include "Assets/MeshAsset.h"
#include "Components/TransformComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "ECS/Entity.h"
#include "Rendering/BGFX/BGFXTypes.hpp"
#include "Rendering/BGFX/BGFXVertex.hpp"
#include "Types/PackedArray.h"
#include "Types/RingBuffer.h"
#include "Input/InputEvent.h"
#include "Types/DeletionQueue.hpp"
#include "Assets/TextureAsset.h"
#include "UI/Editor/Windows/UIWindow.h"

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

	const static std::unordered_map<Assets::TextureFormat, bgfx::TextureFormat::Enum> g_texFormatMap =
	{
		{ Assets::TextureFormat::RGBA8, bgfx::TextureFormat::RGBA8 }
	};

	constexpr uint16_t G_MAX_LIGHTS = 16;

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

	class BGFXRenderSystem : public ECS::System, public std::enable_shared_from_this<BGFXRenderSystem>
	{
	public:

		BGFXRenderSystem()
		{
			m_systemInfo.name = "BGFXRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		~BGFXRenderSystem() override {}

		void Init() override;
		void PreStart() override;
		void Start() override {}
		void Update() override;
		void Stop() override;
		void Cleanup() override;

		void OnInputEvent(const Input::InputEvent& inputEvent);

	private:

		uint32_t m_frameCounter = 0;
		int m_windowWidth, m_windowHeight;
		bool m_windowResized = false;

		// Mesh Vars
		MeshData m_cubeMeshData;
		bgfx::ProgramHandle m_cubeProgram;

		bgfx::ProgramHandle m_meshProgram;

		PackedVector<MeshData> m_meshData;
		PackedVector<MeshDrawBatch> m_meshDrawBatches;

		// Texture Vars
		bgfx::UniformHandle m_texAlbedoSampler;
		bgfx::UniformHandle m_texNormalSampler;

		PackedVector<TextureData> m_texAlbedoHandles;
		PackedVector<TextureData> m_texNormalHandles;

		// Light Vars
		LightUniformHandles m_lightUniformHandles;

		EditorCamera m_editorCam;
		CameraMatComponent m_editorCamMats;
		bool m_moveLeft = false;
		bool m_moveRight = false;
		bool m_moveForward = false;
		bool m_moveBackward = false;
		bool m_moveUp = false;
		bool m_moveDown = false;
		bgfx::UniformHandle m_camPosHandle;

		RingBuffer<Input::InputEvent> m_inputEvents;

		DeletionQueue m_deletionQueue;

		void InitBGFX();
		void InitStaticCubeData();
		void InitMeshProgram();
		void InitTexSamplers();
		void InitCamUniforms();
		void InitLightUniforms();

		void ProcessEvents();

		void InitComponents();
		void UpdateComponents();
		void CleanupComponents();

		void Draw();
		void DrawScene();

		void InitMeshComponent(std::shared_ptr<ECS::Entity> entity);
		void CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity);

		void InitEditorCamera();
		void UpdateEditorCamera();
		void UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity);

		void LoadAndInitMesh(UUID meshID);
		static inline bgfx::VertexBufferHandle InitVertexBuffer(const void* vertices, const uint32_t& numVertices, const bgfx::VertexLayout& layout);
		static inline bgfx::IndexBufferHandle InitIndexBuffer(const void* indices, const uint32_t numIndices, bool use32BitIndices = false);

		bgfx::TextureHandle LoadAndInitTexture(UUID texID) const;

		void SetupLightUniformsForDraw() const;

		static void BuildModelTransform(const TransformComponent& transform, float* model);

		static inline void FrameBufferResizeCallback(GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<BGFXRenderSystem*>(glfwGetWindowUserPointer(window));

			app->m_windowResized = true;
			app->m_windowWidth = width;
			app->m_windowHeight = height;
		}
	};
}
