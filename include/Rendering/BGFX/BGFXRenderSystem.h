#pragma once

#include "ECS\System.h"

#include "BGFXTextureArray.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Components/TransformComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "ECS\Entity.h"
#include "Input/InputEvent.h"
#include "BGFXTypes.h"
#include "BGFXVertex.h"
#include "Types\DeletionQueue.h"
#include "Types/PackedArray.h"
#include "Types/RingBuffer.h"
#include "UI/Editor/Windows/UIWindow.h"

#include <memory>

template<>
struct std::hash<std::pair<puffin::UUID, puffin::UUID>>
{
	std::size_t operator()(const std::pair<puffin::UUID, puffin::UUID>& pair) const noexcept
	{
		return hash<uint64_t>()(pair.first) ^ hash<uint64_t>()(pair.second);
	}
};

namespace puffin::rendering
{
	static const VertexPC32 gCubeVertices[] =
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

	static const uint16_t gCubeTriList[] =
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

	const static std::unordered_map<assets::TextureFormat, bgfx::TextureFormat::Enum> gTexFormatBGFX =
	{
		{ assets::TextureFormat::RGBA8, bgfx::TextureFormat::RGBA8 }
	};

	constexpr uint16_t gMaxLightsBGFX = 16;

	using AssetSetMap = std::unordered_map<UUID, std::set<ECS::EntityID>>; // Used for tracking which entities are using each loaded asset
	using MeshMatPair = std::pair<UUID, UUID>;

	struct CameraMatComponent
	{
		float view[16];
		float proj[16];
	};

	static bgfx::ShaderHandle loadShader(const char* filename)
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
			mSystemInfo.name = "BGFXRenderSystem";
		}

		~BGFXRenderSystem() override {}

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::Init, [&]() { init(); }, "BGFXRenderSystem: Init");
			mEngine->registerCallback(core::ExecutionStage::Setup, [&]() { setup(); }, "BGFXRenderSystem: Setup");
			mEngine->registerCallback(core::ExecutionStage::Render, [&]() { render(); }, "BGFXRenderSystem: Render");
			mEngine->registerCallback(core::ExecutionStage::Stop, [&]() { stop(); }, "BGFXRenderSystem: Stop");
			mEngine->registerCallback(core::ExecutionStage::Cleanup, [&]() { cleanup(); }, "BGFXRenderSystem: Cleanup");
		}

		void init();
		void setup();
		void render();
		void stop();
		void cleanup();

		void onInputEvent(const input::InputEvent& inputEvent);

	private:

		uint32_t mFrameCounter = 0;
		int mWindowWidth;
		int mWindowHeight;
		bool mWindowResized = false;

		bool mUseInstancing = false;
		bool mSupportsInstancing = false;

		// Mesh Vars
		MeshDataBGFX mCubeMeshData;
		bgfx::ProgramHandle mCubeProgram;

		bgfx::ProgramHandle mMeshProgram;
		bgfx::ProgramHandle mMeshInstancedProgram;

		PackedVector<MeshDataBGFX> mMeshData;
		PackedVector<TextureDataBGFX> mTexData;
		PackedVector<MaterialDataBGFX> mMatData;

		AssetSetMap mMeshSets;
		AssetSetMap mTexSets;
		AssetSetMap mMatSets;

		// Texture Vars
		bgfx::UniformHandle mTexAlbedoSampler;
		bgfx::UniformHandle mTexNormalSampler;

		//PackedVector<TextureData> m_texAlbedoHandles;
		//PackedVector<TextureData> m_texNormalHandles;

		//TextureArray m_texAlbedoArray;

		// Light Vars
		LightUniformHandles mLightUniformHandles;

		EditorCamera mEditorCam;
		CameraMatComponent mEditorCamMats;
		bool mMoveLeft = false;
		bool mMoveRight = false;
		bool mMoveForward = false;
		bool mMoveBackward = false;
		bool mMoveUp = false;
		bool mMoveDown = false;
		bgfx::UniformHandle mCamPosHandle;

		RingBuffer<input::InputEvent> mInputEvents;

		DeletionQueue mDeletionQueue;

		void initBGFX();
		void initStaticCubeData();
		void initMeshProgram();
		void initMeshInstancedProgram();
		void initTexSamplers();
		void initCamUniforms();
		void initLightUniforms();

		void processEvents();

		void initComponents();
		void updateComponents();
		void cleanupComponents();

		void draw();
		void drawScene();
		void buildBatches(std::vector<MeshDrawBatch>& batches);
		void drawMeshBatch(const MeshDrawBatch& meshDrawBatch) const;
		void drawMeshBatchInstanced(const MeshDrawBatch& meshDrawBatch);

		void initMeshComponent(std::shared_ptr<ECS::Entity> entity);
		void cleanupMeshComponent(std::shared_ptr<ECS::Entity> entity);

		void initEditorCamera();
		void updateEditorCamera();
		void updateCameraComponent(const std::shared_ptr<ECS::Entity>& entity) const;

		static inline void loadAndInitMesh(UUID meshId, MeshDataBGFX& meshData);
		static inline bgfx::VertexBufferHandle initVertexBuffer(const void* vertices, const uint32_t& numVertices, const bgfx::VertexLayout& layout);
		static inline bgfx::IndexBufferHandle initIndexBuffer(const void* indices, const uint32_t numIndices, bool use32BitIndices = false);

		static inline void loadAndInitTexture(UUID texId, TextureDataBGFX& texData);

		void setupLightUniformsForDraw() const;

		static void buildModelTransform(const TransformComponent& transform, float* model);

		static void frameBufferResizeCallback(GLFWwindow* window, const int width, const int height)
		{
			auto system = reinterpret_cast<BGFXRenderSystem*>(glfwGetWindowUserPointer(window));

			system->mWindowResized = true;
			system->mWindowWidth = width;
			system->mWindowHeight = height;
		}
	};
}
