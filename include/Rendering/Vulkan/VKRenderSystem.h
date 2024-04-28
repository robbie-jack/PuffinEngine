#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

#include "GLFW/glfw3.h"

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "vk_mem_alloc.hpp"

#include "Components/Rendering/CameraComponent.h"
#include "puffin/core/engine.h"
#include "puffin/core/system.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/input/input_event.h""
#include "puffin/assets/texture_asset.h"
#include "puffin/types/deletion_queue.h"
#include "puffin/types/packed_array.h"
#include "puffin/types/ring_buffer.h"
#include "VKDescriptors.h"
#include "VKMaterialRegistry.h"
#include "VKPipeline.h"
#include "VKTypes.h"
#include "VKUnifiedGeometryBuffer.h"

#ifdef NDEBUG
constexpr bool gEnableValidationLayers = false;
#else
constexpr bool gEnableValidationLayers = true;
#endif

namespace puffin
{
	namespace maths
	{
		struct Quat;
	}

	struct TransformComponent3D;
}

namespace puffin::rendering
{
	// Struct containing render data that is static between frames
	struct StaticRenderData
	{
		std::shared_ptr<util::DescriptorAllocator> descriptorAllocator = nullptr;
		std::shared_ptr<util::DescriptorLayoutCache> descriptorLayoutCache = nullptr;

		vk::DescriptorSetLayout globalSetLayout;
		vk::DescriptorSetLayout materialSetLayout;

		vk::Sampler textureSampler;

		UnifiedGeometryBuffer combinedMeshBuffer;
	};

	// Struct containing data that changes each frame
	struct FrameRenderData
	{
		// Synchronization
		vk::Semaphore renderSemaphore;
		vk::Semaphore copySemaphore;
		vk::Semaphore imguiSemaphore;
		vk::Semaphore presentSemaphore;

		vk::Fence renderFence;

		// Command Execution
		vk::CommandPool commandPool;
		vk::CommandBuffer mainCommandBuffer;
		vk::CommandBuffer copyCommandBuffer; // Cmd buffer for copying/blitting from offscreen to swapchain
		vk::CommandBuffer imguiCommandBuffer;

		AllocatedBuffer indirectBuffer; // Buffer of indirect draw commands
		uint32_t drawCount = 0;

		// Global Data (Set for entire frame)
		vk::DescriptorSet globalDescriptor;

		AllocatedBuffer cameraBuffer;
		AllocatedBuffer objectBuffer;
		AllocatedBuffer lightBuffer;
		AllocatedBuffer lightStaticBuffer;
		AllocatedBuffer materialBuffer;
		
		// Material Data (Set for each unique material i.e textures)

		std::unordered_set<PuffinID> renderedMeshes; // Set of meshes last rendered using this data

		bool swapchainNeedsUpdated = false;
		bool offscreenNeedsUpdated = false;
		bool textureDescriptorNeedsUpdated = false;
		bool copyObjectDataToGPU = false;
		bool copyMaterialDataToGPU = false;
	};

	const static std::unordered_map<assets::TextureFormat, vk::Format> gTexFormatVK =
	{
		{ assets::TextureFormat::R8, vk::Format::eR8Unorm },
		{ assets::TextureFormat::RG8, vk::Format::eR8G8Unorm },
		{ assets::TextureFormat::RGB8, vk::Format::eR8G8B8Unorm },
		{ assets::TextureFormat::RGBA8, vk::Format::eR8G8B8A8Unorm },
		{ assets::TextureFormat::BC4, vk::Format::eBc4UnormBlock },
		{ assets::TextureFormat::BC5, vk::Format::eBc5UnormBlock },
		{ assets::TextureFormat::BC7, vk::Format::eBc7UnormBlock }
	};

	constexpr uint32_t gBufferedFrames = 2;
	constexpr uint32_t gMaxObjects = 20000;
	constexpr uint32_t gMaxMaterials = 128;
	constexpr uint32_t gMaxUniqueMaterials = 32;
	constexpr uint32_t gMaxLightsVK = 8;

	// Vulkan Rendering System
	class VKRenderSystem final : public core::System, public std::enable_shared_from_this<VKRenderSystem>
	{
	public:

		VKRenderSystem(const std::shared_ptr<core::Engine>& engine);
		~VKRenderSystem() override { mEngine = nullptr; }

		void startup();
		void render();
		void shutdown();

		const vma::Allocator& allocator() const { return mAllocator ;}
		const vk::Device& device() const { return mDevice; }
		const UploadContext& uploadContext() const { return mUploadContext; }
		const vk::Queue& graphicsQueue() const { return mGraphicsQueue; }
		const StaticRenderData& staticRenderData() const { return mStaticRenderData; }
		const vk::PipelineCache& pipelineCache() const { return mPipelineCache; }
		const OffscreenData& offscreenData() const { return mOffscreenData; }
		const vk::Extent2D& windowSize() const { return mWindowSize; }
		DeletionQueue& deletionQueue() { return mDeletionQueue; }
		bool isReBarEnabled() const { return mIsReBarEnabled; }

		void onUpdateMesh(entt::registry& registry, entt::entity entity);
		void onUpdateTransform(entt::registry& registry, entt::entity entity);
		void onDestroyMeshOrTransform(entt::registry& registry, entt::entity entity);

		void addRenderable(entt::registry& registry, entt::entity entity);

		void registerTexture(PuffinID texID);

	private:

		// Initialization Members
		vk::Device mDevice;
		vk::Instance mInstance;
		vk::PhysicalDevice mPhysicalDevice;
		vk::SurfaceKHR mSurface;
		vk::DebugUtilsMessengerEXT mDebugMessenger;

		vk::Extent2D mWindowSize;
		vk::Extent2D mRenderExtent;

		vma::Allocator mAllocator;

		// Swapchain
		SwapchainData mSwapchainData;
		SwapchainData mOldSwapchainData;

		OffscreenData mOffscreenData;
		OffscreenData mOldOffscreenData;

		// Command Execution
		vk::Queue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;

		StaticRenderData mStaticRenderData;
		std::array<FrameRenderData, gBufferedFrames> mFrameRenderData;

		VKMaterialRegistry mMaterialRegistry;

		std::unordered_set<PuffinID> mMeshesToLoad; // Meshes that need to be loaded
		std::unordered_set<PuffinID> mTexturesToLoad; // Textures that need to be loaded

		PackedVector<TextureDataVK> mTexData;

		std::vector<MeshRenderable> mRenderables; // Renderables data
		bool mUpdateRenderables = false;

		std::vector<MeshDrawBatch> mDrawBatches;

		PackedVector<GPUObjectData> mCachedObjectData; // Cached data for rendering each object in scene
		std::unordered_set<PuffinID> mObjectsToRefresh; // Objects which need their mesh data refreshed

		//PackedVector<GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		uint32_t mFrameNumber;
		uint32_t mDrawCalls = 0;

		// Pipelines
		vk::PipelineCache mPipelineCache;

		util::ShaderModule mForwardVertMod;
		util::ShaderModule mForwardFragMod;
		vk::UniquePipelineLayout mForwardPipelineLayout;
		vk::UniquePipeline mForwardPipeline;

		UploadContext mUploadContext;

		DeletionQueue mDeletionQueue;

		EditorCamera mEditorCam;

		bool mIsInitialized = false; // Indicated initialization completed without any failures
		bool mIsReBarEnabled = false; // Is ReBAR support enabled (Memory heap which is device local and host visible covers all GPU memory)

		void initVulkan();

		void initSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);
		void initOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
		                   const int& offscreenImageCount);

		void initCommands();
		void initSyncStructures();
		void initBuffers();
		void initSamplers();
		void initDescriptors();
		void initPipelines();

		void buildForwardRendererPipeline();

		void initImGui();
		void initOffscreenImGuiTextures(OffscreenData& offscreenData);

		void processComponents();
		void updateCameraComponent(const TransformComponent3D& transform, CameraComponent& camera) const;

		void updateEditorCamera();

		void updateRenderData();

		void draw();

		void recreateSwapchain();
		void cleanSwapchain(SwapchainData& swapchainData);

		void recreateOffscreen();
		void cleanOffscreen(OffscreenData& offscreenData);

		void updateCameras();

		void updateTextureDescriptors();

		void prepareSceneData();
		void prepareMaterialData();
		void prepareObjectData();
		void prepareLightData();

		void buildIndirectCommands();

		vk::CommandBuffer recordMainCommandBuffer(const uint32_t& swapchainIdx, const vk::Extent2D& renderExtent,
		                                          const AllocatedImage&
		                                          colorImage, const AllocatedImage& depthImage);
		void drawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void setDrawParameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void bindBuffersAndDescriptors(vk::CommandBuffer cmd);
		void drawMeshBatch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch);
		void drawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer, vk::DeviceSize offset,
		                                uint32_t drawCount, uint32_t stride);

		vk::CommandBuffer recordCopyCommandBuffer(uint32_t swapchainIdx);
		vk::CommandBuffer recordImGuiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent);

		void recordAndSubmitCommands(uint32_t swapchainIdx);

		static void buildModelTransform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
		                                glm::mat4& model);

		//bool loadMesh(PuffinID meshId, MeshDataVK& meshData);
		//void unloadMesh(MeshDataVK& meshData) const;

		bool loadTexture(PuffinID texId, TextureDataVK& texData);
		void unloadTexture(TextureDataVK& texData) const;

		void buildTextureDescriptorInfo(PackedVector<TextureDataVK>& textureData,
		                                std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;

		FrameRenderData& getCurrentFrameData()
		{
			return mFrameRenderData[mFrameNumber % gBufferedFrames];
		}

		static void frameBufferResizeCallback(GLFWwindow* window, const int width, const int height)
		{
			const auto system = static_cast<VKRenderSystem*>(glfwGetWindowUserPointer(window));

			system->mSwapchainData.resized = true;
			system->mOffscreenData.resized = true;
			system->mWindowSize.width = width;
			system->mWindowSize.height = height;
		}
	};
}
