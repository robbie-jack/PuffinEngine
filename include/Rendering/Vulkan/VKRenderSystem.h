#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.hpp"

#include "VKDescriptors.h"
#include "VKPipeline.h"
#include "VKTypes.h"
#include "VKUnifiedGeometryBuffer.h"
#include "Assets/TextureAsset.h"
#include "Core/Engine.h"
#include "Core/System.h"
#include "ECS/EnTTSubsystem.h"
#include "Input/InputEvent.h"
#include "Types/DeletionQueue.h"
#include "Types/RingBuffer.h"
#include "Types/PackedArray.h"

#include <unordered_set>

#include "Components/Rendering/CameraComponent.h"

#ifdef NDEBUG
constexpr bool gEnableValidationLayers = false;
#else
namespace puffin
{
	struct TransformComponent;
}

constexpr bool gEnableValidationLayers = true;
#endif

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
		
		// Material Data (Set for each unique material i.e textures)
		vk::DescriptorSet materialDescriptor;

		std::unordered_set<PuffinID> renderedMeshes; // Set of meshes last rendered using this data

		bool swapchainNeedsUpdated = false;
		bool offscreenNeedsUpdated = false;
		bool textureDescriptorNeedsUpdated = false;
		bool copyObjectDataToGPU = false;
	};

	const static std::unordered_map<assets::TextureFormat, vk::Format> gTexFormatVK =
	{
		{ assets::TextureFormat::RGBA8, vk::Format::eR8G8B8A8Unorm }
	};

	constexpr uint32_t gBufferedFrames = 2;
	constexpr uint32_t gMaxObjects = 20000;
	constexpr uint32_t gMaxLightsVK = 8;

	// Vulkan Rendering System
	class VKRenderSystem final : public core::System, public std::enable_shared_from_this<VKRenderSystem>
	{
	public:

		VKRenderSystem()
		{
			mSystemInfo.name = "VKRenderSystem";
		}

		void setupCallbacks() override;

		void init();
		void setup();
		void render();
		void cleanup();

		const vma::Allocator& allocator() const { return mAllocator ;}
		const vk::Device& device() const { return mDevice; }
		const UploadContext& uploadContext() const { return mUploadContext; }
		const vk::Queue& graphicsQueue() const { return mGraphicsQueue; }
		bool isReBarEnabled() const { return mIsReBarEnabled; }

		void onInputEvent(const input::InputEvent& inputEvent);

		void onConstructMesh(entt::registry& registry, entt::entity entity);
		void onUpdateMesh(entt::registry& registry, entt::entity entity);
		void onUpdateTransform(entt::registry& registry, entt::entity entity);

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

		vk::RenderPass mRenderPassImGui;

		StaticRenderData mStaticRenderData;
		std::array<FrameRenderData, gBufferedFrames> mFrameRenderData;

		PackedVector<TextureDataVK> mTexData;

		std::unordered_set<PuffinID> mMeshesToLoad; // Meshes that need to be loaded
		std::unordered_set<PuffinID> mTexturesToLoad; // Meshes that need to be loaded
		std::unordered_set<PuffinID> mMaterialsToLoad; // Meshes that need to be loaded

		std::vector<MeshRenderable> mRenderables; // Renderables data
		bool mUpdateRenderables = false;

		PackedVector<GPUObjectData> mObjectData; // Data for rendering each object in scene
		std::unordered_set<PuffinID> mObjectsToRefresh; // Objects which need their mesh data refreshed

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
		bool mMoveLeft = false;
		bool mMoveRight = false;
		bool mMoveForward = false;
		bool mMoveBackward = false;
		bool mMoveUp = false;
		bool mMoveDown = false;

		RingBuffer<input::InputEvent> mInputEvents;

		bool mIsInitialized = false; // Indicated initialization completed without any failures
		bool mIsReBarEnabled = false; // Is ReBAR support enabled (Memory heap which is device local and host visible covers all GPU memory)

		void initVulkan();

		void initSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);
		void initOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
		                   const int& offscreenImageCount);

		void initSwapchainFramebuffers(SwapchainData& swapchainData);

		void initCommands();

		void initImGuiRenderPass();

		void initSyncStructures();
		void initBuffers();
		void initSamplers();
		void initDescriptors();
		void initPipelines();

		void buildForwardRendererPipeline();

		void initImGui();
		void initOffscreenImGuiTextures(OffscreenData& offscreenData);

		void processEvents();

		void processComponents();
		void updateCameraComponent(const TransformComponent& transform, CameraComponent& camera) const;

		void updateEditorCamera();

		void updateRenderData();

		void updateDescriptors();

		void draw();

		void recreateSwapchain();
		void cleanSwapchain(SwapchainData& swapchainData);

		void recreateOffscreen();
		void cleanOffscreen(OffscreenData& offscreenData);

		void updateCameras();

		void updateTextureDescriptors();

		void prepareSceneData();
		void prepareObjectData();
		void prepareLightData();

		void buildIndirectCommands();

		vk::CommandBuffer recordMainCommandBuffer(const uint32_t& swapchainIdx, const vk::Extent2D& renderExtent,
		                                          const AllocatedImage&
		                                          colorImage, const AllocatedImage& depthImage);
		void drawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void drawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer, vk::DeviceSize offset,
		                                uint32_t drawCount, uint32_t stride);

		vk::CommandBuffer recordCopyCommandBuffer(uint32_t swapchainIdx);
		vk::CommandBuffer recordImGuiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent,
		                                           vk::Framebuffer framebuffer);

		void recordAndSubmitCommands(uint32_t swapchainIdx);

		static void buildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale,
		                                glm::mat4& model);

		bool loadMesh(PuffinID meshId, MeshDataVK& meshData);
		void unloadMesh(MeshDataVK& meshData) const;

		bool loadTexture(PuffinID texId, TextureDataVK& texData);
		void unloadTexture(TextureDataVK& texData) const;

		void buildTextureDescriptorInfo(PackedVector<TextureDataVK>& texData,
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
