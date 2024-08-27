#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "vk_mem_alloc.hpp"

#include "puffin/assets/textureasset.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/rendering/materialglobals.h"
#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/rendersubsystem.h"
#include "puffin/rendering/vulkan/descriptorsvk.h"
#include "puffin/rendering/vulkan/pipelinevk.h"
#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/deletionqueue.h"
#include "puffin/types/storage/mappedarray.h"
#include "puffin/types/storage/mappedvector.h"
#include "puffin/types/storage/ringbuffer.h"
#include "puffin/rendering/vulkan/rendergraph/rendergraphvk.h"

#ifdef NDEBUG
constexpr bool gEnableValidationLayers = false;
#else
constexpr bool gEnableValidationLayers = true;
#endif

struct GLFWwindow;

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
	class ResourceManagerVK;
	class MaterialRegistryVK;

	// Struct containing render data that is static between frames
	struct GlobalRenderData
	{
		std::shared_ptr<util::DescriptorAllocator> descriptorAllocator = nullptr;
		std::shared_ptr<util::DescriptorLayoutCache> descriptorLayoutCache = nullptr;

		vk::DescriptorSetLayout objectSetLayout;
		vk::DescriptorSetLayout globalSetLayout;
		vk::DescriptorSetLayout textureSetLayout;
		vk::DescriptorSetLayout shadowmapSetLayout;
		vk::DescriptorSetLayout materialSetLayout;

		vk::Sampler textureSampler;
		vk::Sampler shadowmapSampler;
	};

	// Struct containing data that changes each frame
	struct FrameRenderData
	{
		// Synchronization
		vk::Semaphore shadowSemaphore;
		vk::Semaphore renderSemaphore;
		vk::Semaphore copySemaphore;
		vk::Semaphore imguiSemaphore;
		vk::Semaphore presentSemaphore;

		vk::Fence renderFence;
		vk::Fence presentFence;

		// Command Execution
		vk::CommandPool commandPool;
		vk::CommandBuffer shadowCommandBuffer;
		vk::CommandBuffer mainCommandBuffer;
		vk::CommandBuffer copyCommandBuffer; // Cmd buffer for copying/blitting from offscreen to swapchain
		vk::CommandBuffer imguiCommandBuffer;

		AllocatedBuffer indirectDrawBuffer; // Buffer of indirect draw commands
		uint32_t drawCount = 0;

		// Global Data (Set for entire frame)
		vk::DescriptorSet objectDescriptor;
		vk::DescriptorSet globalDescriptor;
		vk::DescriptorSet textureDescriptor;
		vk::DescriptorSet shadowmapDescriptor;

		AllocatedBuffer cameraBuffer;
		AllocatedBuffer objectBuffer;
		AllocatedBuffer pointLightBuffer;
		AllocatedBuffer spotLightBuffer;
		AllocatedBuffer directionalLightBuffer;
        AllocatedBuffer shadowBuffer;
        AllocatedBuffer shadowCascadeBuffer;
		AllocatedBuffer materialInstanceBuffer;

		GPUFragShaderPushConstant pushConstantFrag;

		// Material Data (Set for each unique material i.e textures)

		std::unordered_set<UUID> renderedMeshes; // Set of meshes last rendered using this data

		bool swapchainNeedsUpdated = false;
		bool offscreenNeedsUpdated = false;
		bool textureDescriptorNeedsUpdated = false;
		bool shadowDescriptorNeedsUpdated = false;
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

	// Vulkan Rendering System
	class RenderSubsystemVK final : public RenderSubsystem, public std::enable_shared_from_this<RenderSubsystemVK>
	{
	public:

		explicit RenderSubsystemVK(const std::shared_ptr<core::Engine>& engine);
		~RenderSubsystemVK() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		double WaitForLastPresentationAndSampleTime() override;
		void Render(double deltaTime) override;

		const vma::Allocator& GetAllocator() const { return mAllocator ;}
		const vk::Device& GetDevice() const { return mDevice; }
		const UploadContext& GetUploadContext() const { return mUploadContext; }
		const vk::Queue& GetGraphicsQueue() const { return mGraphicsQueue; }
		const GlobalRenderData& GetGlobalRenderData() const { return mGlobalRenderData; }
		const vk::PipelineCache& GetPipelineCache() const { return m_pipeline_cache; }
		const OffscreenData& GetOffscreenData() const { return mOffscreenData; }
		const vk::Extent2D& GetWindowSize() const { return mWindowSize; }
		const vk::Extent2D& GetRenderExtent() const { return mRenderExtent; }
		DeletionQueue& GetDeletionQueue() { return mDeletionQueue; }
		bool GetRebarEnabled() const { return mRebarEnabled; }

		void OnUpdateMesh(entt::registry& registry, entt::entity entity);
		void OnUpdateTransform(entt::registry& registry, entt::entity entity);
		void OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity);

		void AddRenderable(entt::registry& registry, entt::entity entity);

		void OnConstructShadowCaster(entt::registry& registry, entt::entity entity);
		void OnUpdateShadowCaster(entt::registry& registry, entt::entity entity);
		void OnDestroyShadowCaster(entt::registry& registry, entt::entity entity);

		void RegisterTexture(UUID textureID);

		[[nodiscard]] uint8_t GetCurrentFrameIdx() const { return m_frame_count % mFramesInFlightCount; }
		[[nodiscard]] uint8_t GetFramesInFlightCount() const { return mFramesInFlightCount; }

	private:

		void InitVulkan();

		void InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);
		void InitOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
		                   const int& offscreenImageCount);

		void InitCommands();
		void InitSyncStructures();
		void InitBuffers();
		void InitSamplers();
		void BuildDescriptors();
		void InitPipelines();
		void BuildForwardRendererPipeline();
		void BuildShadowPipeline();
		void InitImgui();
		void InitOffscreenImguiTextures(OffscreenData& offscreenData);

		void ProcessComponents();

		void UpdateRenderData();

		void ConstructShadows();
		void UpdateShadows();
		void DestroyShadows();

		void Draw();

		void RecreateSwapchain();
		void CleanSwapchain(SwapchainData& swapchainData);

		void RecreateOffscreen();
		void CleanOffscreen(OffscreenData& offscreenData);

		void UpdateTextureDescriptors();
		void UpdateShadowDescriptors();

		void PrepareSceneData();
		void PrepareMaterialData();
		void PrepareObjectData();
		void PrepareLightData();
		void PrepareShadowData();

		void BuildIndirectCommands();

		vk::CommandBuffer& RecordShadowCommandBuffer(uint32_t swapchainIdx);
		void DrawShadowmap(vk::CommandBuffer cmd, const AllocatedImage& depthImage, const vk::Extent2D& shadowExtent);

		struct RecordMainCommandBufferParams
		{
			uint32_t swapchainIdx;
			vk::Extent2D renderExtent;
			AllocatedImage colorImage;
			AllocatedImage depthImage;
		};
		vk::CommandBuffer& RecordMainCommandBuffer(
			const RecordMainCommandBufferParams& params);
		void DrawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void SetDrawParameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void BindBuffersAndDescriptors(vk::CommandBuffer cmd);
		void DrawMeshBatch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch);
		void DrawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer, vk::DeviceSize offset,
		                                uint32_t drawCount, uint32_t stride);

		vk::CommandBuffer& RecordCopyCommandBuffer(uint32_t swapchainIdx);
		vk::CommandBuffer& RecordImguiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent);

		void RecordAndSubmitCommands(uint32_t swapchainIdx);

		static void BuildModelTransform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
		                                glm::mat4& model);

		bool LoadTexture(UUID texId, TextureDataVK& texData);
		void UnloadTexture(TextureDataVK& texData) const;

		void BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
		                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;
		void BuildShadowDescriptorInfo(std::vector<vk::DescriptorImageInfo>& shadowImageInfos);

		FrameRenderData& GetCurrentFrameData();

		static void FrameBufferResizeCallback(GLFWwindow* window, const int width, const int height);

		struct ShadowConstructEvent
		{
			entt::entity entity;
			ImageDesc imageDesc;
		};

		struct ShadowUpdateEvent
		{
			entt::entity entity;
			ImageDesc imageDesc;
			uint8_t frameCount = 0;
		};

		struct ShadowDestroyEvent
		{
			ResourceID resourceID;
			uint8_t frameCount = 0;
		};

		// Initialization Members
		RenderGraphVK mRenderGraph;
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
		SwapchainData mSwapchainDataOld;

		OffscreenData mOffscreenData;
		OffscreenData mOffscreenDataOld;

		// Command Execution
		vk::Queue mGraphicsQueue = {};
		uint32_t mGraphicsQueueFamily = 0;

		GlobalRenderData mGlobalRenderData;
		std::array<FrameRenderData, gBufferedFrames> mFrameRenderData;

		std::unique_ptr<ResourceManagerVK> mResourceManager = nullptr;
		std::unique_ptr<MaterialRegistryVK> mMaterialRegistry = nullptr;

		std::unordered_set<UUID> mMeshesToLoad; // Meshes that need to be loaded
		std::unordered_set<UUID> mTexturesToLoad; // Textures that need to be loaded

		MappedVector<UUID, TextureDataVK> mTexData;

		std::vector<MeshRenderable> mRenderables; // Renderables data
		bool mUpdateRenderables = false;

		std::vector<MeshDrawBatch> mDrawBatches;

		MappedVector<UUID, GPUObjectData> mCachedObjectData; // Cached data for rendering each object in scene
		std::unordered_set<UUID> mObjectsToRefresh; // Objects which need their mesh data refreshed

		//MappedVector<GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		MappedVector<UUID, Vector3f> mCachedLightDirection;

		RingBuffer<ShadowConstructEvent> mShadowConstructEvents;
		RingBuffer<ShadowUpdateEvent> mShadowUpdateEvents;
		RingBuffer<ShadowDestroyEvent> mShadowDestroyEvents;
		std::vector<UUID> mShadowsToDraw;

		uint32_t mCurrentSwapchainIdx = 0;
		uint8_t mFramesInFlightCount = gBufferedFrames;
		uint32_t m_frame_count;
		uint32_t m_draw_calls = 0;

		// Pipelines
		vk::PipelineCache m_pipeline_cache;

		util::ShaderModule mForwardVertMod;
		util::ShaderModule mForwardFragMod;
		vk::UniquePipelineLayout mForwardPipelineLayout;
		vk::UniquePipeline mForwardPipeline;

		util::ShaderModule mShadowVertMod;
		util::ShaderModule mShadowFragMod;
		vk::UniquePipelineLayout mShadowPipelineLayout;
		vk::UniquePipeline mShadowPipeline;

		UploadContext mUploadContext;

		DeletionQueue mDeletionQueue;

		bool mInitialized = false; // Indicates initialization completed without any failures
		bool mRebarEnabled = false; // Is ReBAR support enabled (Memory heap which is device local and host visible covers all GPU memory)
		bool mRenderShadows = false; // Render shadows if enabled
	};
}
