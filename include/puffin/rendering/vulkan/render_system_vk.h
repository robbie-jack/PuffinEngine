#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

#include "GLFW/glfw3.h"

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "vk_mem_alloc.hpp"

#include "puffin/core/engine.h"
#include "puffin/core/system.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/assets/texture_asset.h"
#include "puffin/types/deletion_queue.h"
#include "puffin/types/packed_array.h"
#include "puffin/rendering/vulkan/descriptors_vk.h"
#include "puffin/components/rendering/camera_component.h"
#include "puffin/rendering/vulkan/unified_geometry_buffer_vk.h"
#include "puffin/rendering/material_globals.h"
#include "puffin/rendering/vulkan/material_registry_vk.h"
#include "puffin/rendering/vulkan/pipeline_vk.h"

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
	class ResourceManagerVK;

	// Struct containing render data that is static between frames
	struct GlobalRenderData
	{
		std::shared_ptr<util::DescriptorAllocator> descriptor_allocator = nullptr;
		std::shared_ptr<util::DescriptorLayoutCache> descriptor_layout_cache = nullptr;

		vk::DescriptorSetLayout global_set_layout;
		vk::DescriptorSetLayout material_set_layout;

		vk::Sampler texture_sampler;
	};

	// Struct containing data that changes each frame
	struct FrameRenderData
	{
		// Synchronization
		vk::Semaphore shadow_semaphore;
		vk::Semaphore render_semaphore;
		vk::Semaphore copy_semaphore;
		vk::Semaphore imgui_semaphore;
		vk::Semaphore present_semaphore;

		vk::Fence render_fence;

		// Command Execution
		vk::CommandPool command_pool;
		vk::CommandBuffer shadow_command_buffer;
		vk::CommandBuffer main_command_buffer;
		vk::CommandBuffer copy_command_buffer; // Cmd buffer for copying/blitting from offscreen to swapchain
		vk::CommandBuffer imgui_command_buffer;

		AllocatedBuffer indirect_buffer; // Buffer of indirect draw commands
		uint32_t draw_count = 0;

		// Global Data (Set for entire frame)
		vk::DescriptorSet global_descriptor;

		AllocatedBuffer camera_buffer;
		AllocatedBuffer object_buffer;
		AllocatedBuffer light_buffer;
		AllocatedBuffer light_static_buffer;
		AllocatedBuffer material_buffer;

		// Material Data (Set for each unique material i.e textures)

		std::unordered_set<PuffinID> rendered_meshes; // Set of meshes last rendered using this data

		bool swapchain_needs_updated = false;
		bool offscreen_needs_updated = false;
		bool texture_descriptor_needs_updated = false;
		bool copy_object_data_to_gpu = false;
		bool copy_material_data_to_gpu = false;
	};

	const static std::unordered_map<assets::TextureFormat, vk::Format> g_tex_format_vk =
	{
		{ assets::TextureFormat::R8, vk::Format::eR8Unorm },
		{ assets::TextureFormat::RG8, vk::Format::eR8G8Unorm },
		{ assets::TextureFormat::RGB8, vk::Format::eR8G8B8Unorm },
		{ assets::TextureFormat::RGBA8, vk::Format::eR8G8B8A8Unorm },
		{ assets::TextureFormat::BC4, vk::Format::eBc4UnormBlock },
		{ assets::TextureFormat::BC5, vk::Format::eBc5UnormBlock },
		{ assets::TextureFormat::BC7, vk::Format::eBc7UnormBlock }
	};

	constexpr uint32_t g_buffered_frames = 2;
	constexpr uint32_t g_max_objects = 20000;
	constexpr uint32_t g_max_materials = 128;
	constexpr uint32_t g_max_unique_materials = 32;
	constexpr uint32_t g_max_lights_vk = 8;

	// Vulkan Rendering System
	class RenderSystemVK final : public core::System, public std::enable_shared_from_this<RenderSystemVK>
	{
	public:

		RenderSystemVK(const std::shared_ptr<core::Engine>& engine);
		~RenderSystemVK() override { mEngine = nullptr; }

		void startup();
		void render();
		void shutdown();

		const vma::Allocator& allocator() const { return m_allocator ;}
		const vk::Device& device() const { return m_device; }
		const UploadContext& upload_context() const { return m_upload_context; }
		const vk::Queue& graphics_queue() const { return m_graphics_queue; }
		const GlobalRenderData& static_render_data() const { return m_global_render_data; }
		const vk::PipelineCache& pipeline_cache() const { return m_pipeline_cache; }
		const OffscreenData& offscreen_data() const { return m_offscreen_data; }
		const vk::Extent2D& window_size() const { return m_window_size; }
		DeletionQueue& deletion_queue() { return m_deletion_queue; }
		bool rebar_enabled() const { return m_rebar_enabled; }

		void on_update_mesh(entt::registry& registry, entt::entity entity);
		void on_update_transform(entt::registry& registry, entt::entity entity);
		void on_destroy_mesh_or_transform(entt::registry& registry, entt::entity entity);

		void add_renderable(entt::registry& registry, entt::entity entity);

		void on_update_shadow_caster(entt::registry& registry, entt::entity entity);
		void on_destroy_shadow_caster(entt::registry& registry, entt::entity entity);

		void register_texture(PuffinID texID);

	private:

		// Initialization Members
		vk::Device m_device;
		vk::Instance m_instance;
		vk::PhysicalDevice m_physical_device;
		vk::SurfaceKHR m_surface;
		vk::DebugUtilsMessengerEXT m_debug_messenger;

		vk::Extent2D m_window_size;
		vk::Extent2D m_render_extent;

		vma::Allocator m_allocator;

		// Swapchain
		SwapchainData m_swapchain_data;
		SwapchainData m_swapchain_data_old;

		OffscreenData m_offscreen_data;
		OffscreenData m_offscreen_data_old;

		// Command Execution
		vk::Queue m_graphics_queue;
		uint32_t m_graphics_queue_family;

		GlobalRenderData m_global_render_data;
		std::array<FrameRenderData, g_buffered_frames> m_frame_render_data;

		ResourceManagerVK* m_resource_manager = nullptr;
		VKMaterialRegistry m_material_registry;

		std::unordered_set<PuffinID> m_meshes_to_load; // Meshes that need to be loaded
		std::unordered_set<PuffinID> m_textures_to_load; // Textures that need to be loaded

		PackedVector<PuffinID, TextureDataVK> m_tex_data;

		std::vector<MeshRenderable> m_renderables; // Renderables data
		bool m_update_renderables = false;

		std::vector<MeshDrawBatch> m_draw_batches;

		PackedVector<PuffinID, GPUObjectData> m_cached_object_data; // Cached data for rendering each object in scene
		std::unordered_set<PuffinID> m_objects_to_refresh; // Objects which need their mesh data refreshed

		//PackedVector<GPUMaterialInstanceData> mCachedMaterialData; // Cached data for each unique material/instance

		uint32_t m_frame_number;
		uint32_t m_draw_calls = 0;

		// Pipelines
		vk::PipelineCache m_pipeline_cache;

		util::ShaderModule m_forward_vert_mod;
		util::ShaderModule m_forward_frag_mod;
		vk::UniquePipelineLayout m_forward_pipeline_layout;
		vk::UniquePipeline m_forward_pipeline;

		UploadContext m_upload_context;

		DeletionQueue m_deletion_queue;

		EditorCamera m_editor_cam;

		bool m_initialized = false; // Indicates initialization completed without any failures
		bool m_rebar_enabled = false; // Is ReBAR support enabled (Memory heap which is device local and host visible covers all GPU memory)
		bool m_render_shadows = false; // Render shadows if enabled

		void init_vulkan();

		void init_swapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
		                   const vk::Extent2D& swapchainExtent);
		void init_offscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
		                   const int& offscreenImageCount);

		void init_commands();
		void init_sync_structures();
		void init_buffers();
		void init_samplers();
		void init_descriptors();
		void init_pipelines();

		void build_forward_renderer_pipeline();

		void init_imgui();
		void init_offscreen_imgui_textures(OffscreenData& offscreenData);

		void process_components();

		void update_render_data();

		void draw();

		void recreate_swapchain();
		void clean_swapchain(SwapchainData& swapchainData);

		void recreate_offscreen();
		void clean_offscreen(OffscreenData& offscreenData);

		void update_cameras();
		void update_editor_camera();
		void update_camera_component(const TransformComponent3D& transform, CameraComponent& camera) const;

		void update_texture_descriptors();

		void prepare_scene_data();
		void prepare_material_data();
		void prepare_object_data();
		void prepare_light_data();

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

		bool load_texture(PuffinID texId, TextureDataVK& texData);
		void unload_texture(TextureDataVK& texData) const;

		void build_texture_descriptor_info(PackedVector<PuffinID, TextureDataVK>& textureData,
		                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;

		FrameRenderData& current_frame_data()
		{
			return m_frame_render_data[m_frame_number % g_buffered_frames];
		}

		static void frame_buffer_resize_callback(GLFWwindow* window, const int width, const int height)
		{
			const auto system = static_cast<RenderSystemVK*>(glfwGetWindowUserPointer(window));

			system->m_swapchain_data.resized = true;
			system->m_offscreen_data.resized = true;
			system->m_window_size.width = width;
			system->m_window_size.height = height;
		}
	};
}
