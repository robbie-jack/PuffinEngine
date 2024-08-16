#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "vk_mem_alloc.hpp"

#include "puffin/assets/textureasset.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/rendering/materialglobals.h"
#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/resourceid.h"
#include "puffin/core/subsystem.h"
#include "puffin/rendering/vulkan/descriptorsvk.h"
#include "puffin/rendering/vulkan/pipelinevk.h"
#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/deletionqueue.h"
#include "puffin/types/packedarray.h"
#include "puffin/types/packedvector.h"
#include "puffin/types/ringbuffer.h"

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
		AllocatedBuffer lightBuffer;
		AllocatedBuffer pointLightBuffer;
		AllocatedBuffer directionalLightBuffer;
		AllocatedBuffer spotLightBuffer;
        AllocatedBuffer shadowBuffer;
        AllocatedBuffer shadowCascadeBuffer;
		AllocatedBuffer materialBuffer;

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
	class RenderSubystemVK final : public core::Subsystem, public std::enable_shared_from_this<RenderSubystemVK>
	{
	public:

		explicit RenderSubystemVK(const std::shared_ptr<core::Engine>& engine);
		~RenderSubystemVK() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		double WaitForLastPresentationAndSampleTime() override;
		void Render(double deltaTime) override;

		const vma::Allocator& GetAllocator() const { return mAllocator ;}
		const vk::Device& GetDevice() const { return mDevice; }
		const UploadContext& GetUploadContext() const { return m_upload_context; }
		const vk::Queue& GetGraphicsQueue() const { return mGraphicsQueue; }
		const GlobalRenderData& GetGlobalRenderData() const { return mGlobalRenderData; }
		const vk::PipelineCache& GetPipelineCache() const { return m_pipeline_cache; }
		const OffscreenData& GetOffscreenData() const { return mOffscreenData; }
		const vk::Extent2D& GetWindowSize() const { return mWindowSize; }
		const vk::Extent2D& GetRenderExtent() const { return mRenderExtent; }
		DeletionQueue& GetDeletionQueue() { return m_deletion_queue; }
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

		void init_vulkan();

		void init_swapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);
		void init_offscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
		                   const int& offscreenImageCount);

		void init_commands();
		void init_sync_structures();
		void init_buffers();
		void init_samplers();
		void build_descriptors();
		void init_pipelines();

		void build_forward_renderer_pipeline();
		void build_shadow_pipeline();

		void init_imgui();
		void init_offscreen_imgui_textures(OffscreenData& offscreenData);

		void process_components();

		void update_render_data();

		void construct_shadows();
		void update_shadows();
		void destroy_shadows();

		void draw();

		void recreate_swapchain();
		void clean_swapchain(SwapchainData& swapchainData);

		void recreate_offscreen();
		void clean_offscreen(OffscreenData& offscreenData);

		void update_texture_descriptors();
		void update_shadow_descriptors();

		void prepare_scene_data();
		void prepare_material_data();
		void prepare_object_data();
		void prepare_light_data();
		void prepare_shadow_data();

		void build_indirect_commands();

		vk::CommandBuffer& record_shadow_command_buffer(uint32_t swapchain_idx);
		void draw_shadowmap(vk::CommandBuffer cmd, const AllocatedImage& depth_image, const vk::Extent2D& shadow_extent);

		vk::CommandBuffer& record_main_command_buffer(const uint32_t& swapchain_idx, const vk::Extent2D& render_extent,
		                                              const AllocatedImage&
		                                              color_image, const AllocatedImage& depth_image);
		void draw_objects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void set_draw_parameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent);
		void bind_buffers_and_descriptors(vk::CommandBuffer cmd);
		void draw_mesh_batch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch);
		void draw_indexed_indirect_command(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer, vk::DeviceSize offset,
		                                uint32_t drawCount, uint32_t stride);

		vk::CommandBuffer& record_copy_command_buffer(uint32_t swapchainIdx);
		vk::CommandBuffer& record_imgui_command_buffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent);

		void record_and_submit_commands(uint32_t swapchainIdx);

		static void build_model_transform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
		                                glm::mat4& model);

		//bool loadMesh(PuffinID meshId, MeshDataVK& meshData);
		//void unloadMesh(MeshDataVK& meshData) const;

		bool load_texture(UUID texId, TextureDataVK& texData);
		void unload_texture(TextureDataVK& texData) const;

		void build_texture_descriptor_info(PackedVector<UUID, TextureDataVK>& textureData,
		                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;
		void build_shadow_descriptor_info(std::vector<vk::DescriptorImageInfo>& shadowImageInfos);

		FrameRenderData& current_frame_data();

		static void frame_buffer_resize_callback(GLFWwindow* window, const int width, const int height);

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
		std::array<FrameRenderData, g_buffered_frames> mFrameRenderData;

		std::unique_ptr<ResourceManagerVK> mResourceManager = nullptr;
		std::unique_ptr<MaterialRegistryVK> mMaterialRegistry = nullptr;

		std::unordered_set<UUID> mMeshesToLoad; // Meshes that need to be loaded
		std::unordered_set<UUID> mTexturesToLoad; // Textures that need to be loaded

		PackedVector<UUID, TextureDataVK> mTexData;

		std::vector<MeshRenderable> mRenderables; // Renderables data
		bool mUpdateRenderables = false;

		std::vector<MeshDrawBatch> mDrawBatches;

		PackedVector<UUID, GPUObjectData> mCachedObjectData; // Cached data for rendering each object in scene
		std::unordered_set<UUID> mObjectsToRefresh; // Objects which need their mesh data refreshed

		//PackedVector<GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		RingBuffer<ShadowConstructEvent> mShadowConstructEvents;
		RingBuffer<ShadowUpdateEvent> mShadowUpdateEvents;
		RingBuffer<ShadowDestroyEvent> mShadowDestroyEvents;
		std::vector<UUID> mShadowsToDraw;

		uint32_t mCurrentSwapchainIdx = 0;
		uint8_t mFramesInFlightCount = g_buffered_frames;
		uint32_t m_frame_count;
		uint32_t m_draw_calls = 0;

		// Pipelines
		vk::PipelineCache m_pipeline_cache;

		util::ShaderModule mForwardVertMod;
		util::ShaderModule m_forward_frag_mod;
		vk::UniquePipelineLayout m_forward_pipeline_layout;
		vk::UniquePipeline m_forward_pipeline;

		util::ShaderModule m_shadow_vert_mod;
		util::ShaderModule m_shadow_frag_mod;
		vk::UniquePipelineLayout m_shadow_pipeline_layout;
		vk::UniquePipeline m_shadow_pipeline;

		UploadContext m_upload_context;

		DeletionQueue m_deletion_queue;

		bool mInitialized = false; // Indicates initialization completed without any failures
		bool mRebarEnabled = false; // Is ReBAR support enabled (Memory heap which is device local and host visible covers all GPU memory)
		bool mRenderShadows = false; // Render shadows if enabled
	};
}
