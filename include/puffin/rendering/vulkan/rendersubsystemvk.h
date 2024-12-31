#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

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
#include "rendermodule/rendermodulevk.h"

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
	class TextureManagerVK;
	class UnifiedGeometryBufferVK;
	class RenderModuleVK;
	class ResourceManagerVK;
	class MaterialRegistryVK;

	// Struct containing render data that is static between frames
	struct GlobalRenderData
	{
		vk::DescriptorSetLayout objectSetLayout;
		vk::DescriptorSetLayout lightSetLayout;
		vk::DescriptorSetLayout materialSetLayout;
		vk::DescriptorSetLayout shadowSetLayout;

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

		std::unordered_map<std::string, vk::Semaphore> semaphores;

		vk::Fence renderFence;
		vk::Fence presentFence;

		// Command Execution
		vk::CommandPool commandPool;
		vk::CommandBuffer shadowCommandBuffer;
		vk::CommandBuffer mainCommandBuffer;
		vk::CommandBuffer copyCommandBuffer; // Cmd buffer for copying/blitting from offscreen to swapchain
		vk::CommandBuffer imguiCommandBuffer;

		std::unordered_map<std::string, vk::CommandBuffer> commandBuffers;

		AllocatedBuffer indirectDrawBuffer; // Buffer of indirect draw commands
		uint32_t drawCount = 0;
		uint64_t presentID = 0;

		// Global Data (Set for entire frame)
		vk::DescriptorSet objectDescriptor;
		vk::DescriptorSet lightDescriptor;
		vk::DescriptorSet materialDescriptor;
		vk::DescriptorSet shadowDescriptor;

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

		const vk::Instance& GetInstance() const { return mInstance; }
		const vma::Allocator& GetAllocator() const { return mAllocator ;}
		const vk::Device& GetDevice() const { return mDevice; }
		const UploadContext& GetUploadContext() const { return mUploadContext; }
		const vk::Queue& GetGraphicsQueue() const { return mGraphicsQueue; }
		const GlobalRenderData& GetGlobalRenderData() const { return mGlobalRenderData; }
		const vk::PipelineCache& GetPipelineCache() const { return mPipelineCache; }
		const OffscreenData& GetOffscreenData() const { return mOffscreenData; }
		const vk::Extent2D& GetSwapchainExtent() const { return mSwapchainExtent; }
		const vk::Extent2D& GetRenderExtent() const { return mRenderExtent; }
		DeletionQueue& GetDeletionQueue() { return mDeletionQueue; }
		bool GetRebarEnabled() const { return mRebarEnabled; }
		ResourceManagerVK* GetResourceManager() const { return mResourceManager.get(); }
		TextureManagerVK* GetTextureManager() const { return mTextureManager.get(); }
		MaterialRegistryVK* GetMaterialRegistry() const { return mMaterialRegistry.get(); }
		UnifiedGeometryBufferVK* GetUnifiedGeometryBuffer() const { return mUnifiedGeometryBuffer.get(); }
		util::DescriptorAllocator* GetDescriptorAllocator() const { return mDescriptorAllocator.get(); }
		util::DescriptorLayoutCache* GetDescriptorLayoutCache() const { return mDescriptorLayoutCache.get(); }
		

		void OnUpdateMesh(entt::registry& registry, entt::entity entity);
		void OnUpdateTransform(entt::registry& registry, entt::entity entity);
		void OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity);

		void AddRenderable(entt::registry& registry, entt::entity entity);

		void OnConstructOrUpdateShadowCaster(entt::registry& registry, entt::entity entity);
		void OnDestroyShadowCaster(entt::registry& registry, entt::entity entity);

		struct DrawCommandParams
		{
			uint32_t vertexCount = 0;
			uint32_t instanceCount = 0;
			uint32_t firstVertex = 0;
			uint32_t firstInstance = 0;
		};
		void DrawCommand(vk::CommandBuffer& cmd, const DrawCommandParams& cmdParams);

		struct DrawIndexedCommandParams
		{
			uint32_t indexCount = 0;
			uint32_t instanceCount = 0;
			uint32_t firstIndex = 0;
			uint32_t vertexOffset = 0;
			uint32_t firstInstance = 0;
		};
		void DrawIndexedCommand(vk::CommandBuffer& cmd, const DrawIndexedCommandParams& cmdParams);

		struct DrawIndirectCommandParams
		{
			vk::Buffer buffer;
			vk::DeviceSize offset = 0;
			uint32_t drawCount = 0;
			uint32_t stride = 0;
		};
		void DrawIndirectCommand(vk::CommandBuffer& cmd, const DrawIndirectCommandParams& cmdParams);
		void DrawIndexedIndirectCommand(vk::CommandBuffer& cmd, const DrawIndirectCommandParams& cmdParams);

		[[nodiscard]] uint8_t GetCurrentFrameIdx() const { return mFrameIdx; }
		[[nodiscard]] uint8_t GetNextFrameIdx() const { return (mFrameIdx + 1) % mFramesInFlightCount; }
		[[nodiscard]] uint8_t GetOffsetFrameIdx(uint8_t offset) const { return (mFrameIdx + offset) % mFramesInFlightCount; }
		[[nodiscard]] uint8_t GetFramesInFlightCount() const { return mFramesInFlightCount; }

		template<typename T>
		void RegisterModule(const std::string& moduleName)
		{
			if (mRenderModules.find(moduleName) != mRenderModules.end())
				return;

			mRenderModules.emplace(moduleName, new T(mEngine, this));

			mRenderModules.at(moduleName)->RegisterModules();

			mRenderModuleVector.push_back(mRenderModules.at(moduleName));
		}

		template<typename T>
		T* GetModule(const std::string& moduleName)
		{
			if (mRenderModules.find(moduleName) != mRenderModules.end())
				return static_cast<T*>(mRenderModules.at(moduleName));

			return nullptr;
		}

	private:

		// Initialization Methods
		void InitVulkan();

		void InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);

		void InitModules();

		void PostInitModules();

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

		// Deinitialization Methods

		void DeinitModules();

		// Pre-Render Methods
		void UpdateResources();

		void UpdateGraph();
		void UpdateGraphSemaphores();
		void UpdateGraphCommands();

		void PreRender(double deltaTime);

		void ProcessComponents();

		void UpdateRenderData();

		void UpdateShadows();

		// Render Methods

		void WaitForRenderFence();

		void GetNextSwapchainIdx();

		void UpdateSwapchainAndOffscreen();

		void RenderEditorUI();

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

		void RecordRenderPassCommands();

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

		vk::CommandBuffer& RecordCopyCommandBuffer(uint32_t swapchainIdx);
		vk::CommandBuffer& RecordImguiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent);

		void RecordAndSubmitCommands(uint32_t swapchainIdx);

		void BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
		                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;
		void BuildShadowDescriptorInfo(std::vector<vk::DescriptorImageInfo>& shadowImageInfos);

		FrameRenderData& GetFrameData(uint8_t frameIdx);
		FrameRenderData& GetCurrentFrameData();

		static void FrameBufferResizeCallback(GLFWwindow* window, const int width, const int height);

		// Initialization Members
		RenderGraphVK mRenderGraph;
		std::unordered_map<std::string, RenderModuleVK*> mRenderModules;
		std::vector<RenderModuleVK*> mRenderModuleVector; // Vector of render module pointers, in ordered modules were registered

		vk::Device mDevice;
		vk::Instance mInstance;
		vk::PhysicalDevice mPhysicalDevice;
		vk::SurfaceKHR mSurface;
		vk::DebugUtilsMessengerEXT mDebugMessenger;

		vk::Extent2D mSwapchainExtent;
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
		std::array<FrameRenderData, gBufferedFrameCount> mFrameRenderData;

		std::unique_ptr<ResourceManagerVK> mResourceManager = nullptr;
		std::unique_ptr<TextureManagerVK> mTextureManager = nullptr;
		std::unique_ptr<UnifiedGeometryBufferVK> mUnifiedGeometryBuffer = nullptr;
		std::unique_ptr<MaterialRegistryVK> mMaterialRegistry = nullptr;
		std::unique_ptr<util::DescriptorAllocator> mDescriptorAllocator = nullptr;
		std::unique_ptr<util::DescriptorLayoutCache> mDescriptorLayoutCache = nullptr;

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

		std::unordered_set<UUID> mShadowsToUpdate;
		std::unordered_set<ResourceID> mShadowResourcesToDestroy;
		std::vector<UUID> mShadowsToDraw;

		uint32_t mCurrentSwapchainIdx = 0;
		uint8_t mFramesInFlightCount = gBufferedFrameCount;
		uint8_t mFrameIdx = 0;
		uint64_t mFrameCount = 0;
		uint64_t mNextPresentID = 1;

		// Pipelines
		vk::PipelineCache mPipelineCache;

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
		bool mShadowsEnabled = false; // Render shadows if enabled
		bool mPresentWaitEnabled = false; // VK_KHR_present_wait extension is supported and enabled
		bool mDescriptorBuffersEnabled = false;
	};
}
