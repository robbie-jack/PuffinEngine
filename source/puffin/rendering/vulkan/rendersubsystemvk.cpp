#include "puffin/rendering/vulkan/rendersubsystemvk.h"

#include <iostream>

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include "GLFW/glfw3.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "puffin/mathhelpers.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "puffin/core/engine.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/rendering/lightcomponent.h"
#include "puffin/components/rendering/meshcomponent.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/rendering/materialglobals.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/scene/scenegraph.h"
#include "puffin/editor/ui/editoruisubsystem.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
#include "puffin/rendering/vulkan/materialregistryvk.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/editor/ui/windows/uiwindowviewport.h"
#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/renderhelpers.h"

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		vk::Result err = x;                                         \
		if (err != vk::Result::eSuccess)                            \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace puffin::rendering
{
	RenderSubystemVK::RenderSubystemVK(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "RenderSubystemVK";
	}

	RenderSubystemVK::~RenderSubystemVK()
	{
	}

	void RenderSubystemVK::Initialize(core::SubsystemManager* subsystem_manager)
	{
		// Bind callbacks
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		registry->on_construct<MeshComponent>().connect<&RenderSubystemVK::on_update_mesh>(this);
		registry->on_update<MeshComponent>().connect<&RenderSubystemVK::on_update_mesh>(this);
		registry->on_destroy<MeshComponent>().connect<&RenderSubystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<TransformComponent2D>().connect<&RenderSubystemVK::on_update_transform>(this);
		registry->on_update<TransformComponent2D>().connect<&RenderSubystemVK::on_update_transform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&RenderSubystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<TransformComponent3D>().connect<&RenderSubystemVK::on_update_transform>(this);
		registry->on_update<TransformComponent3D>().connect<&RenderSubystemVK::on_update_transform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&RenderSubystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<ShadowCasterComponent>().connect<&RenderSubystemVK::on_construct_shadow_caster>(this);
		registry->on_update<ShadowCasterComponent>().connect<&RenderSubystemVK::on_update_shadow_caster>(this);
		registry->on_destroy<ShadowCasterComponent>().connect<&RenderSubystemVK::on_destroy_shadow_caster>(this);

		auto signal_subsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
		auto rendering_draw_shadows_signal = signal_subsystem->GetSignal<bool>("rendering_draw_shadows");
		if (!rendering_draw_shadows_signal)
		{
			rendering_draw_shadows_signal = signal_subsystem->CreateSignal<bool>("rendering_draw_shadows");
		}

		rendering_draw_shadows_signal->Connect(std::function([&](const bool& rendering_draw_shadows)
		{
			m_render_shadows = rendering_draw_shadows;
		}));

		// Initialise vulkan and all rendering objects
		init_vulkan();

		init_swapchain(m_swapchain_data, m_swapchain_data_old.swapchain, m_window_size);

		if (mEngine->GetShouldRenderEditorUI())
		{
			auto editor_ui_subsystem = subsystem_manager->CreateAndInitializeSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewport_size = editor_ui_subsystem->window_viewport()->viewportSize();
			m_render_extent.width = viewport_size.x;
			m_render_extent.height = viewport_size.y;
		}
		else
		{
			m_render_extent = m_window_size;
		}

		init_offscreen(m_offscreen_data, m_render_extent, m_swapchain_data.images.size());

		init_commands();

		init_sync_structures();
		init_buffers();
		init_samplers();

		build_descriptors();
		init_pipelines();

		if (mEngine->GetShouldRenderEditorUI())
		{
			init_imgui();
			init_offscreen_imgui_textures(m_offscreen_data);
		}

		m_renderables.reserve(g_max_objects);
		m_cached_object_data.reserve(g_max_objects);

		m_resource_manager = std::make_unique<ResourceManagerVK>(this);
		m_material_registry = std::make_unique<MaterialRegistryVK>(this);

		m_initialized = true;
		m_update_renderables = true;

		const auto settings_manager = mEngine->GetSubsystem<core::SettingsManager>();
		m_render_shadows = settings_manager->get<bool>("rendering_draw_shadows");
	}

	void RenderSubystemVK::Deinitialize()
	{
		m_device.waitIdle();

		if (m_initialized)
		{
			for (auto texData : m_tex_data)
			{
				unload_texture(texData);
			}

			m_tex_data.clear();

			clean_swapchain(m_swapchain_data);

			if (m_swapchain_data_old.needs_cleaned)
			{
				clean_swapchain(m_swapchain_data_old);
			}

			clean_offscreen(m_offscreen_data);

			if (m_offscreen_data_old.needs_cleaned)
			{
				clean_offscreen(m_offscreen_data_old);
			}

			m_resource_manager = nullptr;
			//m_material_registry = nullptr;

			m_deletion_queue.flush();

			m_initialized = false;
		}
	}

	core::SubsystemType RenderSubystemVK::GetType() const
	{
		return core::SubsystemType::Render;
	}

	double RenderSubystemVK::WaitForLastPresentationAndSampleTime()
	{
		// Wait until GPU has finished presenting last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &current_frame_data().present_fence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &current_frame_data().present_fence));

		VK_CHECK(m_device.acquireNextImageKHR(m_swapchain_data.swapchain, 1000000000, current_frame_data().present_semaphore,
				current_frame_data().present_fence, &m_current_swapchain_idx));

		return glfwGetTime();
	}

	void RenderSubystemVK::Render(double delta_time)
	{
		update_render_data();

		process_components();

		draw();
	}

	void RenderSubystemVK::on_update_mesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<MeshComponent>(entity);

		if (mesh.mesh_asset_id == gInvalidID || mesh.mat_asset_id == gInvalidID)
		{
			return;
		}

		m_meshes_to_load.insert(mesh.mesh_asset_id);
		m_material_registry->register_material_instance(mesh.mat_asset_id);

		add_renderable(registry, entity);
	}

	void RenderSubystemVK::on_update_transform(entt::registry& registry, entt::entity entity)
	{
		add_renderable(registry, entity);
	}

	void RenderSubystemVK::on_destroy_mesh_or_transform(entt::registry& registry, entt::entity entity)
	{
		m_update_renderables = true;
	}

	void RenderSubystemVK::add_renderable(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<TransformComponent2D, TransformComponent3D>(entity) && registry.any_of<MeshComponent>(entity))
		{
			auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto id = entt_subsystem->get_id(entity);
			const auto mesh = registry.get<MeshComponent>(entity);

			if (mesh.mesh_asset_id == gInvalidID || mesh.mat_asset_id == gInvalidID)
			{
				return;
			}

			m_objects_to_refresh.insert(id);

			m_update_renderables = true;
		}
	}

	void RenderSubystemVK::on_construct_shadow_caster(entt::registry& registry, entt::entity entity)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = entt_subsystem->get_id(entity);
		const auto& shadow = registry.get<ShadowCasterComponent>(entity);

		ImageDesc image_desc;
		image_desc.image_type = ImageType::Depth;
		image_desc.format = vk::Format::eD32Sfloat;
		image_desc.width = shadow.width;
		image_desc.height = shadow.height;
		image_desc.depth = 1;

		m_shadow_construct_events.push({ entity, image_desc });
	}

	void RenderSubystemVK::on_update_shadow_caster(entt::registry& registry, entt::entity entity)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = entt_subsystem->get_id(entity);
		const auto& shadow = registry.get<ShadowCasterComponent>(entity);

		ImageDesc image_desc;
		image_desc.image_type = ImageType::Depth;
		image_desc.format = vk::Format::eD32Sfloat;
		image_desc.width = shadow.width;
		image_desc.height = shadow.height;
		image_desc.depth = 1;

		m_shadow_update_events.push({ entity, image_desc });
	}

	void RenderSubystemVK::on_destroy_shadow_caster(entt::registry& registry, entt::entity entity)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto id = entt_subsystem->get_id(entity);
		auto& shadow = registry.get<ShadowCasterComponent>(entity);

		m_shadow_destroy_events.push({ shadow.resource_id });

		shadow.resource_id = gInvalidID;
	}

	void RenderSubystemVK::register_texture(PuffinID tex_id)
	{
		m_textures_to_load.insert(tex_id);
	}

	void RenderSubystemVK::init_vulkan()
	{
		auto window_subsystem = mEngine->GetSubsystem<window::WindowSubsystem>();
		GLFWwindow* glfw_window = window_subsystem->primary_window();

		glfwSetWindowUserPointer(glfw_window, this);
		glfwSetFramebufferSizeCallback(glfw_window, frame_buffer_resize_callback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfw_window, &width, &height);
		m_window_size.width = static_cast<unsigned>(width);
		m_window_size.height = static_cast<unsigned>(height);

		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
		                          .request_validation_layers(gEnableValidationLayers)
		                          .require_api_version(1, 3, 25)
		                          .use_default_debug_messenger()
		                          .enable_extension("VK_KHR_get_physical_device_properties2")
		                          .build();

		vkb::Instance vkbInst = instRet.value();

		// Store Instance/Debug Messenger
		m_instance = vkbInst.instance;
		m_debug_messenger = vkbInst.debug_messenger;

		// Get Window Surface
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(m_instance, glfw_window, nullptr, &surface);
		m_surface = surface;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing = true;
		physicalDeviceFeatures.drawIndirectFirstInstance = true;
		physicalDeviceFeatures.multiDrawIndirect = true;
		physicalDeviceFeatures.samplerAnisotropy = true;

		vk::PhysicalDeviceVulkan12Features physicalDevice12Features = {};
		physicalDevice12Features.descriptorIndexing = true;
		physicalDevice12Features.runtimeDescriptorArray = true;
		physicalDevice12Features.descriptorBindingVariableDescriptorCount = true;
		physicalDevice12Features.descriptorBindingPartiallyBound = true;
		physicalDevice12Features.bufferDeviceAddress = true;

		vk::PhysicalDeviceVulkan13Features physicalDevice13Features = {};
		physicalDevice13Features.dynamicRendering = true;

		vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value();

		// Check for desired extension support
		std::vector<const char*> device_extensions =
		{
			"VK_EXT_memory_budget",
		};

		// Select GPU
		vkb::PhysicalDeviceSelector selector{vkbInst};
		vkb::PhysicalDevice physDevice = selector
		.set_minimum_version(1, 3)
		.set_surface(m_surface)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_12(physicalDevice12Features)
		.set_required_features_13(physicalDevice13Features)
		.add_required_extensions(device_extensions)
		.select()
		.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder{physDevice};

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {true};

		vkb::Device vkbDevice = deviceBuilder
		                        .add_pNext(&shaderDrawParametersFeatures)
		                        .build()
		                        .value();

		m_device = vkbDevice.device;
		m_physical_device = physDevice.physical_device;

		// Get Graphics Queue
		m_graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// Init memory allocator
		vma::AllocatorCreateInfo allocatorInfo = {vma::AllocatorCreateFlagBits::eBufferDeviceAddress, m_physical_device, m_device};
		allocatorInfo.instance = m_instance;

		VK_CHECK(vma::createAllocator(&allocatorInfo, &m_allocator));

		// Check memory types
		const vk::PhysicalDeviceMemoryProperties* deviceMemProps = m_allocator.getMemoryProperties();

		for (int i = 0; i < deviceMemProps->memoryTypeCount; i++)
		{
			const vk::MemoryType& memType = deviceMemProps->memoryTypes[i];
			const vk::MemoryPropertyFlags& memPropFlags = memType.propertyFlags;

			if (memPropFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal))
			{
				vk::DeviceSize heapSize = deviceMemProps->memoryHeaps[memType.heapIndex].size;
				vk::DeviceSize heapSizeMB = heapSize / 1024 / 1024;

				if (heapSizeMB > 256)
				{
					m_rebar_enabled = true;
				}
			}
		}

		m_deletion_queue.pushFunction([=]()
		{
			m_allocator.destroy();

			m_device.destroy();
			m_instance.destroySurfaceKHR(m_surface);
			vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
			m_instance.destroy();
		});
	}

	void RenderSubystemVK::init_swapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
	                                   const vk::Extent2D& swapchainExtent)
	{
		vkb::SwapchainBuilder swapchainBuilder{m_physical_device, m_device, m_surface};

		swapchainData.extent = swapchainExtent;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
		                              .use_default_format_selection()
		                              // Vsync present mode
		                              .set_old_swapchain(oldSwapchain)
		                              .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		                              .set_desired_extent(swapchainData.extent.width, swapchainData.extent.height)
                .set_image_usage_flags(
			                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    )
		                              .build()
		                              .value();

		swapchainData.swapchain = vkbSwapchain.swapchain;
		swapchainData.image_format = static_cast<vk::Format>(vkbSwapchain.image_format);

		// Grab Images
		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		swapchainData.images.resize(images.size());
		swapchainData.image_views.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			swapchainData.images[i] = static_cast<vk::Image>(images[i]);
			swapchainData.image_views[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();
	}

	void RenderSubystemVK::init_offscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
	                                   const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = {offscreenData.extent.width, offscreenData.extent.height, 1};

		offscreenData.image_format = vk::Format::eR8G8B8A8Unorm;

		const vk::ImageCreateInfo imageInfo = {
			{}, vk::ImageType::e2D, offscreenData.image_format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
			}
		};

		const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		const vk::ImageViewCreateInfo imageViewInfo = {
			{}, {}, vk::ImageViewType::e2D, offscreenData.image_format, {}, subresourceRange
		};

		offscreenData.alloc_images.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.alloc_images[i] = util::create_image(this, imageInfo, imageViewInfo);
		}

		offscreenData.alloc_depth_image = util::create_depth_image(this, imageExtent, vk::Format::eD32Sfloat);
	}

	void RenderSubystemVK::init_commands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_queue_family
		};
		vk::CommandBufferAllocateInfo commandBufferInfo = {{}, vk::CommandBufferLevel::ePrimary, 1};

		// Init Main Command Pools/Buffers
		for (int i = 0; i < g_buffered_frames; i++)
		{
			VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_frame_render_data[i].command_pool));

			commandBufferInfo.commandPool = m_frame_render_data[i].command_pool;
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frame_render_data[i].shadow_command_buffer));
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frame_render_data[i].main_command_buffer));
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frame_render_data[i].copy_command_buffer));
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frame_render_data[i].imgui_command_buffer));

			m_deletion_queue.pushFunction([=]()
			{
				m_device.destroyCommandPool(m_frame_render_data[i].command_pool);
			});
		}

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphics_queue_family};
		VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_upload_context.commandPool));

		commandBufferInfo = {m_upload_context.commandPool, vk::CommandBufferLevel::ePrimary, 1};
		VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_upload_context.commandBuffer));

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroyCommandPool(m_upload_context.commandPool);
		});
	}

	void RenderSubystemVK::init_sync_structures()
	{
		vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled, nullptr};
		vk::SemaphoreCreateInfo semaphoreCreateInfo = {{}, nullptr};

		for (int i = 0; i < g_buffered_frames; i++)
		{
			VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_frame_render_data[i].render_fence));
			VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_frame_render_data[i].present_fence));

			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].render_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].copy_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].imgui_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].present_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].shadow_semaphore));

			m_deletion_queue.pushFunction([=]()
			{
				m_device.destroyFence(m_frame_render_data[i].render_fence);
				m_device.destroyFence(m_frame_render_data[i].present_fence);

				m_device.destroySemaphore(m_frame_render_data[i].render_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].copy_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].imgui_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].present_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].shadow_semaphore);
			});
		}

		// Init Upload Context Fence
		fenceCreateInfo = vk::FenceCreateInfo{{}, nullptr};
		VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_upload_context.uploadFence));

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroyFence(m_upload_context.uploadFence);
		});
	}

	void RenderSubystemVK::init_buffers()
	{
		for (int i = 0; i < g_buffered_frames; i++)
		{
			// Indirect Buffer
			m_frame_render_data[i].indirect_buffer = util::create_buffer(
				m_allocator, sizeof(vk::DrawIndexedIndirectCommand) * g_max_objects,
				vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
				vk::BufferUsageFlagBits::eTransferDst,
				vma::MemoryUsage::eAuto,
				vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

			// Global Buffers
			m_frame_render_data[i].camera_buffer = util::create_buffer(m_allocator, sizeof(GPUCameraData),
			                                                      vk::BufferUsageFlagBits::eUniformBuffer,
			                                                      vma::MemoryUsage::eAuto,
			                                                      {
				                                                      vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                      | vma::AllocationCreateFlagBits::eMapped
			                                                      });

			m_frame_render_data[i].light_buffer = util::create_buffer(m_allocator, sizeof(GPULightData) * g_max_lights,
			                                                     vk::BufferUsageFlagBits::eStorageBuffer,
			                                                     vma::MemoryUsage::eAuto,
			                                                     {
				                                                     vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                     | vma::AllocationCreateFlagBits::eMapped
			                                                     });

			m_frame_render_data[i].shadow_buffer = util::create_buffer(m_allocator, sizeof(GPUShadowData) * g_max_lights,
			                                                           vk::BufferUsageFlagBits::eStorageBuffer,
			                                                           vma::MemoryUsage::eAuto,
			                                                           {
				                                                           vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                           | vma::AllocationCreateFlagBits::eMapped
			                                                           });

            m_frame_render_data[i].shadow_cascade_buffer = util::create_buffer(m_allocator, sizeof(GPUShadowCascadeData) * g_max_lights * g_max_shadow_cascades_per_light,
                                                                       vk::BufferUsageFlagBits::eStorageBuffer,
                                                                       vma::MemoryUsage::eAuto,
                                                                       {
                                                                               vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
                                                                               | vma::AllocationCreateFlagBits::eMapped
                                                                       });

			m_frame_render_data[i].object_buffer = util::create_buffer(m_allocator, sizeof(GPUObjectData) * g_max_objects,
			                                                      vk::BufferUsageFlagBits::eStorageBuffer,
			                                                      vma::MemoryUsage::eAuto,
			                                                      {
				                                                      vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                      | vma::AllocationCreateFlagBits::eMapped
			                                                      });

			m_frame_render_data[i].material_buffer = util::create_buffer(m_allocator, sizeof(GPUMaterialInstanceData) * g_max_materials,
																	vk::BufferUsageFlagBits::eStorageBuffer,
																	vma::MemoryUsage::eAuto,
																	{
																		vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
																		   | vma::AllocationCreateFlagBits::eMapped
																	});

			// Material Buffers

			// Object Buffers

			m_deletion_queue.pushFunction([=]()
			{
				m_allocator.destroyBuffer(m_frame_render_data[i].material_buffer.buffer,
										 m_frame_render_data[i].material_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].object_buffer.buffer,
				                         m_frame_render_data[i].object_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].shadow_buffer.buffer,
				                         m_frame_render_data[i].shadow_buffer.allocation);
                m_allocator.destroyBuffer(m_frame_render_data[i].shadow_cascade_buffer.buffer,
                                          m_frame_render_data[i].shadow_cascade_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].light_buffer.buffer,
				                         m_frame_render_data[i].light_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].camera_buffer.buffer,
				                         m_frame_render_data[i].camera_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].indirect_buffer.buffer,
				                         m_frame_render_data[i].indirect_buffer.allocation);
			});
		}
	}

	void RenderSubystemVK::init_samplers()
	{
		vk::SamplerCreateInfo texture_sampler_info = {};
		texture_sampler_info.anisotropyEnable = true;
		texture_sampler_info.maxAnisotropy = 16.0f;

		m_global_render_data.texture_sampler = m_device.createSampler(texture_sampler_info);

		vk::SamplerCreateInfo shadowmap_sampler_info = {};
		shadowmap_sampler_info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
		shadowmap_sampler_info.addressModeU = vk::SamplerAddressMode::eClampToBorder;
		shadowmap_sampler_info.addressModeV = vk::SamplerAddressMode::eClampToBorder;
		shadowmap_sampler_info.addressModeW = vk::SamplerAddressMode::eClampToBorder;

		m_global_render_data.shadowmap_sampler = m_device.createSampler(shadowmap_sampler_info);

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroySampler(m_global_render_data.texture_sampler, nullptr);
			m_device.destroySampler(m_global_render_data.shadowmap_sampler, nullptr);
		});
	}

	void RenderSubystemVK::build_descriptors()
	{
		// Descriptor Allocator/Cache

		m_global_render_data.descriptor_allocator = std::make_shared<util::DescriptorAllocator>(m_device);
		m_global_render_data.descriptor_layout_cache = std::make_shared<util::DescriptorLayoutCache>(m_device);

		for (int i = 0; i < g_buffered_frames; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo object_buffer_info = {
				m_frame_render_data[i].object_buffer.buffer, 0, sizeof(GPUObjectData) * g_max_objects
			};

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
				m_global_render_data.descriptor_allocator)
				.bindBuffer(0, &object_buffer_info, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.build(m_frame_render_data[i].object_descriptor, m_global_render_data.object_set_layout);

			vk::DescriptorBufferInfo camera_buffer_info = {
				m_frame_render_data[i].camera_buffer.buffer, 0, sizeof(GPUCameraData)
			};
			vk::DescriptorBufferInfo light_buffer_info = {
				m_frame_render_data[i].light_buffer.buffer, 0, sizeof(GPULightData) * g_max_lights
			};
			vk::DescriptorBufferInfo shadow_buffer_info = {
				m_frame_render_data[i].shadow_buffer.buffer, 0, sizeof(GPUShadowData) * g_max_lights
			};
            vk::DescriptorBufferInfo shadow_cascade_buffer_info = {
                    m_frame_render_data[i].shadow_cascade_buffer.buffer, 0, sizeof(GPUShadowCascadeData) * g_max_lights * g_max_shadow_cascades_per_light
            };
			vk::DescriptorBufferInfo material_buffer_info = {
				m_frame_render_data[i].material_buffer.buffer, 0, sizeof(GPUMaterialInstanceData) * g_max_materials
			};

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
				m_global_render_data.descriptor_allocator)
				.bindBuffer(0, &camera_buffer_info, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(1, &light_buffer_info, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(2, &shadow_buffer_info, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(3, &shadow_cascade_buffer_info, vk::DescriptorType::eStorageBuffer,vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(4, &material_buffer_info, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.build(m_frame_render_data[i].global_descriptor, m_global_render_data.global_set_layout);

			const uint32_t num_images = 128;
			uint32_t variable_desc_counts = num_images;

			constexpr vk::DescriptorBindingFlags descriptor_binding_flags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
			vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_alloc_info =
			{ 1, &variable_desc_counts };

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
				m_global_render_data.descriptor_allocator)
				.bindImagesWithoutWrite(0, num_images, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptor_binding_flags)
				.addPNext(&variable_descriptor_count_alloc_info)
				.build(m_frame_render_data[i].texture_descriptor, m_global_render_data.texture_set_layout);

			variable_desc_counts = g_max_lights;

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
				m_global_render_data.descriptor_allocator)
				.bindImagesWithoutWrite(0, g_max_lights, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptor_binding_flags)
				.addPNext(&variable_descriptor_count_alloc_info)
				.build(m_frame_render_data[i].shadowmap_descriptor, m_global_render_data.shadowmap_set_layout);
		}

		m_deletion_queue.pushFunction([=]()
		{
			m_global_render_data.descriptor_layout_cache = nullptr;
			m_global_render_data.descriptor_allocator = nullptr;
		});
	}

	void RenderSubystemVK::init_pipelines()
	{
		build_forward_renderer_pipeline();
		build_shadow_pipeline();
	}

	void RenderSubystemVK::build_forward_renderer_pipeline()
	{
		m_forward_vert_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_vs.spv").string()
		};

		m_forward_frag_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_fs.spv").string()
		};

		vk::PushConstantRange vert_range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant) };
		vk::PushConstantRange frag_range = { vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUFragShaderPushConstant) };

		util::PipelineLayoutBuilder plb{};
		m_forward_pipeline_layout = plb
		.descriptorSetLayout(m_global_render_data.object_set_layout)
		.descriptorSetLayout(m_global_render_data.global_set_layout)
		.descriptorSetLayout(m_global_render_data.texture_set_layout)
		.descriptorSetLayout(m_global_render_data.shadowmap_set_layout)
		.pushConstantRange(vert_range)
		.pushConstantRange(frag_range)
		.createUnique(m_device);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, m_offscreen_data.image_format, m_offscreen_data.alloc_depth_image.format
		};

		util::PipelineBuilder pb{m_window_size.width, m_window_size.height};
		m_forward_pipeline = pb
		                   // Define dynamic state which can change each frame (currently viewport and scissor size)
		                   .dynamicState(vk::DynamicState::eViewport)
		                   .dynamicState(vk::DynamicState::eScissor)
		                   // Define vertex/fragment shaders
		                   .shader(vk::ShaderStageFlagBits::eVertex, m_forward_vert_mod)
		                   .shader(vk::ShaderStageFlagBits::eFragment, m_forward_frag_mod)
		                   .depthStencilState(depthStencilInfo)
		                   // Add rendering info struct
		                   .addPNext(&pipelineRenderInfo)
		                   // Create pipeline
		                   .createUnique(m_device, m_pipeline_cache, *m_forward_pipeline_layout, nullptr);

		m_device.destroyShaderModule(m_forward_vert_mod.module());
		m_device.destroyShaderModule(m_forward_frag_mod.module());

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroyPipeline(m_forward_pipeline.get());
			m_device.destroyPipelineLayout(m_forward_pipeline_layout.get());

			m_forward_pipeline = {};
			m_forward_pipeline_layout = {};
		});
	}

	void RenderSubystemVK::build_shadow_pipeline()
	{
		m_shadow_vert_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "shadowmaps" / "shadowmap_vs.spv").string()
		};

		m_shadow_frag_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::Get()->GetEngineRoot() / "bin" / "vulkan" / "shadowmaps" / "shadowmap_fs.spv").string()
		};

		vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUShadowPushConstant) };

		util::PipelineLayoutBuilder plb{};
		m_shadow_pipeline_layout = plb
			.descriptorSetLayout(m_global_render_data.object_set_layout)
			.pushConstantRange(range)
			.createUnique(m_device);

		const vk::PipelineDepthStencilStateCreateInfo depth_stencil_info = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipeline_render_info = {
			0, {}, vk::Format::eD32Sfloat
		};


		util::PipelineBuilder pb{ 1024, 1024 };
		m_shadow_pipeline = pb
			// Define dynamic state which can change each frame (currently viewport and scissor size)
			.dynamicState(vk::DynamicState::eViewport)
			.dynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.shader(vk::ShaderStageFlagBits::eVertex, m_shadow_vert_mod)
			.shader(vk::ShaderStageFlagBits::eFragment, m_shadow_frag_mod)
			.depthStencilState(depth_stencil_info)
			// Add rendering info struct
			.addPNext(&pipeline_render_info)
			// Create pipeline
			.createUnique(m_device, m_pipeline_cache, *m_shadow_pipeline_layout, nullptr);

		m_device.destroyShaderModule(m_shadow_frag_mod.module());
		m_device.destroyShaderModule(m_shadow_vert_mod.module());

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroyPipeline(m_shadow_pipeline.get());
			m_device.destroyPipelineLayout(m_shadow_pipeline_layout.get());

			m_shadow_pipeline = {};
			m_shadow_pipeline_layout = {};
		});
	}

	void RenderSubystemVK::init_imgui()
	{
		// Create Descriptor Pool for ImGui
		vk::DescriptorPoolSize poolSizes[] =
		{
			{vk::DescriptorType::eSampler, 1000},
			{vk::DescriptorType::eCombinedImageSampler, 1000},
			{vk::DescriptorType::eSampledImage, 1000},
			{vk::DescriptorType::eStorageImage, 1000},
			{vk::DescriptorType::eUniformTexelBuffer, 1000},
			{vk::DescriptorType::eStorageTexelBuffer, 1000},
			{vk::DescriptorType::eUniformBuffer, 1000},
			{vk::DescriptorType::eStorageBuffer, 1000},
			{vk::DescriptorType::eUniformBufferDynamic, 1000},
			{vk::DescriptorType::eStorageBufferDynamic, 1000},
			{vk::DescriptorType::eInputAttachment, 1000}
		};

		vk::DescriptorPoolCreateInfo poolInfo =
		{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, std::size(poolSizes), poolSizes
		};

		vk::DescriptorPool imguiPool;
		VK_CHECK(m_device.createDescriptorPool(&poolInfo, nullptr, &imguiPool));

		// Initialize imgui for GLFW
		const auto window_subsystem = mEngine->GetSubsystem<window::WindowSubsystem>();
		GLFWwindow* glfw_window = window_subsystem->primary_window();
		ImGui_ImplGlfw_InitForVulkan(glfw_window, true);

		std::array<VkFormat,1 > formats = { static_cast<VkFormat>(m_swapchain_data.image_format) };

		VkPipelineRenderingCreateInfoKHR pipelineRenderInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			nullptr, 0, 1,
			formats.data(), VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED
		};

		// Initialize imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo = {
			m_instance, m_physical_device, m_device, m_graphics_queue_family,
			m_graphics_queue, imguiPool, nullptr, static_cast<uint32_t>(m_swapchain_data.images.size()),
			static_cast<uint32_t>(m_swapchain_data.images.size()), VK_SAMPLE_COUNT_1_BIT, m_pipeline_cache,
			0, true, pipelineRenderInfo
		};

		ImGui_ImplVulkan_Init(&initInfo);

		// Upload ImGui font textures
		util::immediate_submit(this, [=](vk::CommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture();
		});

		// Clear font textures from cpu data
		//ImGui_ImplVulkan_DestroyFontUploadObjects();

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroyDescriptorPool(imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void RenderSubystemVK::init_offscreen_imgui_textures(OffscreenData& offscreenData)
	{
		offscreenData.viewport_textures.resize(offscreenData.alloc_images.size());

		for (int i = 0; i < offscreenData.alloc_images.size(); i++)
		{
			offscreenData.viewport_textures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				m_global_render_data.texture_sampler,
				offscreenData.alloc_images[i].image_view,
				static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void RenderSubystemVK::process_components()
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		if (m_update_renderables)
		{
			const auto meshView2D = registry->view<const TransformComponent2D, const MeshComponent>();
			const auto meshView3D = registry->view<const TransformComponent3D, const MeshComponent>();

			m_renderables.clear();

			// Iterate 2D objects
			for (auto [entity, transform, mesh] : meshView2D.each())
			{
				const auto node_id = entt_subsystem->get_id(entity);

				if (mesh.mat_asset_id == gInvalidID || mesh.mesh_asset_id == gInvalidID)
				{
					continue;
				}

				const auto& matData = m_material_registry->get_material_data(mesh.mat_asset_id);

				m_renderables.emplace_back(node_id, mesh.mesh_asset_id, matData.baseMaterialID, mesh.sub_mesh_idx);

				if (!m_cached_object_data.contains(node_id))
				{
					m_cached_object_data.emplace(node_id, GPUObjectData());
				}
			}

			// Iterate 3D objects
			for (auto [entity, transform, mesh] : meshView3D.each())
			{
				const auto node_id = entt_subsystem->get_id(entity);

				if (mesh.mat_asset_id == gInvalidID || mesh.mesh_asset_id == gInvalidID)
				{
					continue;
				}

				const auto& matData = m_material_registry->get_material_data(mesh.mat_asset_id);

				m_renderables.emplace_back(node_id, mesh.mesh_asset_id, matData.baseMaterialID, mesh.sub_mesh_idx);

				if (!m_cached_object_data.contains(node_id))
				{
					m_cached_object_data.emplace(node_id, GPUObjectData());
				}
			}

			std::sort(m_renderables.begin(), m_renderables.end());

			for (auto& frameData : m_frame_render_data)
			{
				frameData.copy_object_data_to_gpu = true;
			}

			m_update_renderables = false;
		}
	}

	void RenderSubystemVK::update_render_data()
	{
		// Load Meshes
		{
			for (const auto mesh_id : m_meshes_to_load)
			{
				if (const auto static_mesh = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(mesh_id))
				{
					m_resource_manager->add_static_mesh(static_mesh);
				}
			}

			m_meshes_to_load.clear();

			// Load Materials
			m_material_registry->update();

			if (m_material_registry->material_data_needs_uploaded())
			{
				for (uint32_t i = 0; i < g_buffered_frames; i++)
				{
					m_frame_render_data[i].copy_material_data_to_gpu = true;
				}
			}
		}

		// Load Textures
		{
			bool texture_descriptor_needs_updated = false;
			for (const auto tex_id : m_textures_to_load)
			{
				if (tex_id != gInvalidID && !m_tex_data.contains(tex_id))
				{
					TextureDataVK tex_data;
					load_texture(tex_id, tex_data);

					m_tex_data.emplace(tex_id, tex_data);

					texture_descriptor_needs_updated = true;
				}
			}

			m_textures_to_load.clear();

			if (texture_descriptor_needs_updated == true)
			{
				for (int i = 0; i < g_buffered_frames; i++)
				{
					m_frame_render_data[i].texture_descriptor_needs_updated = true;
				}
			}
		}

		construct_shadows();
		update_shadows();
		destroy_shadows();
	}

	void RenderSubystemVK::construct_shadows()
	{
		bool shadow_descriptor_needs_updated = false;

		while(!m_shadow_construct_events.empty())
		{
			ShadowConstructEvent shadow_event{};
			m_shadow_construct_events.pop(shadow_event);

			auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = entt_subsystem->registry();
			auto& shadow = registry->get<ShadowCasterComponent>(shadow_event.entity);

			shadow.resource_id = m_resource_manager->add_images(shadow_event.image_desc, m_frames_in_flight_count * shadow.cascade_count);

			shadow_descriptor_needs_updated |= true;
		}

		if (shadow_descriptor_needs_updated == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].shadow_descriptor_needs_updated = true;
			}
		}
	}

	void RenderSubystemVK::update_shadows()
	{
		bool shadow_descriptor_needs_updated = false;

		std::vector<ShadowUpdateEvent> m_shadow_update_events_still_in_use;
		while (!m_shadow_update_events.empty())
		{
			ShadowUpdateEvent shadow_event{};
			m_shadow_update_events.pop(shadow_event);

			auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			auto registry = entt_subsystem->registry();
			auto& shadow = registry->get<ShadowCasterComponent>(shadow_event.entity);

			if (m_resource_manager->image_exists(shadow.resource_id))
			{
				const int first_image_index = current_frame_idx() * shadow.cascade_count;
				for (int i = 0; i < shadow.cascade_count; ++i)
				{
					m_resource_manager->update_image(shadow.resource_id, shadow_event.image_desc, first_image_index + i);
				}

				shadow_event.frame_count++;

				if (shadow_event.frame_count < m_frames_in_flight_count)
				{
					m_shadow_update_events_still_in_use.push_back(shadow_event);
				}
			}

			shadow_descriptor_needs_updated |= true;
		}

		for (const auto& shadow_event : m_shadow_update_events_still_in_use)
		{
			m_shadow_update_events.push(shadow_event);
		}

		m_shadow_update_events_still_in_use.clear();

		if (shadow_descriptor_needs_updated == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].shadow_descriptor_needs_updated = true;
			}
		}
	}

	void RenderSubystemVK::destroy_shadows()
	{
		std::vector<ShadowDestroyEvent> m_shadow_destroy_events_still_in_use;
		while (!m_shadow_destroy_events.empty())
		{
			ShadowDestroyEvent shadow_event{};
			m_shadow_destroy_events.pop(shadow_event);

			if (m_resource_manager->image_exists(shadow_event.resource_id))
			{
				shadow_event.frame_count++;

				if (shadow_event.frame_count < m_frames_in_flight_count)
				{
					m_shadow_destroy_events_still_in_use.push_back(shadow_event);
				}
				else
				{
					m_resource_manager->destroy_images(shadow_event.resource_id);
				}
			}
		}

		for (const auto& shadow_event : m_shadow_destroy_events_still_in_use)
		{
			m_shadow_destroy_events.push(shadow_event);
		}

		m_shadow_destroy_events_still_in_use.clear();
	}

	void RenderSubystemVK::draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &current_frame_data().render_fence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &current_frame_data().render_fence));

		if (mEngine->GetShouldRenderEditorUI())
		{
			auto editor_ui_subsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			const ImVec2 viewportSize = editor_ui_subsystem->window_viewport()->viewportSize();

			m_render_extent.width = static_cast<uint32_t>(viewportSize.x);
			m_render_extent.height = static_cast<uint32_t>(viewportSize.y);

			if (viewportSize.x != m_offscreen_data.extent.width ||
				viewportSize.y != m_offscreen_data.extent.height)
			{
				m_offscreen_data.resized = true;
			}
		}
		else
		{
			m_render_extent = m_window_size;
		}

		recreate_swapchain();
		recreate_offscreen();

		if (mEngine->GetShouldRenderEditorUI())
		{
			auto editor_ui_subsystem = mEngine->GetSubsystem<ui::EditorUISubsystem>();
			editor_ui_subsystem->window_viewport()->draw(m_offscreen_data.viewport_textures[m_current_swapchain_idx]);

			ImGui::Render();
		}

		m_draw_calls = 0;

		// Prepare textures, scene data & indirect commands for rendering
		update_texture_descriptors();
		update_shadow_descriptors();
		prepare_scene_data();
		build_indirect_commands();

		record_and_submit_commands(m_current_swapchain_idx);

		m_frame_count++;
	}

	void RenderSubystemVK::recreate_swapchain()
	{
		// Recreate swapchain when window is resized
		if (m_swapchain_data.resized == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].swapchain_needs_updated = true;
			}

			m_swapchain_data_old = m_swapchain_data;
			m_swapchain_data_old.needs_cleaned = true;

			init_swapchain(m_swapchain_data, m_swapchain_data_old.swapchain, m_window_size);

			m_swapchain_data.resized = false;
		}

		if (m_swapchain_data_old.needs_cleaned == true)
		{
			if (current_frame_data().swapchain_needs_updated == true)
			{
				current_frame_data().swapchain_needs_updated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < g_buffered_frames; i++)
			{
				if (m_frame_render_data[i].swapchain_needs_updated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == g_buffered_frames)
			{
				clean_swapchain(m_swapchain_data_old);

				m_swapchain_data_old.needs_cleaned = false;
			}
		}
	}

	void RenderSubystemVK::clean_swapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.image_views.size(); i++)
		{
			m_device.destroyImageView(swapchainData.image_views[i]);
		}

		m_device.destroySwapchainKHR(swapchainData.swapchain);
	}

	void RenderSubystemVK::recreate_offscreen()
	{
		if (m_offscreen_data.resized == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].offscreen_needs_updated = true;
			}

			m_offscreen_data_old = m_offscreen_data;
			m_offscreen_data_old.needs_cleaned = true;

			init_offscreen(m_offscreen_data, m_render_extent, m_swapchain_data.images.size());

			if (mEngine->GetShouldRenderEditorUI())
			{
				init_offscreen_imgui_textures(m_offscreen_data);
			}

			m_offscreen_data.resized = false;
		}

		if (m_offscreen_data_old.needs_cleaned == true)
		{
			if (current_frame_data().offscreen_needs_updated == true)
			{
				current_frame_data().offscreen_needs_updated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < g_buffered_frames; i++)
			{
				if (m_frame_render_data[i].offscreen_needs_updated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == g_buffered_frames)
			{
				clean_offscreen(m_offscreen_data_old);

				m_offscreen_data_old.needs_cleaned = false;
			}
		}
	}

	void RenderSubystemVK::clean_offscreen(OffscreenData& offscreenData)
	{
		m_device.destroyImageView(offscreenData.alloc_depth_image.image_view);
		m_allocator.destroyImage(offscreenData.alloc_depth_image.image, offscreenData.alloc_depth_image.allocation);

		for (int i = 0; i < offscreenData.alloc_images.size(); i++)
		{
			if (mEngine->GetShouldRenderEditorUI())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewport_textures[i]));
			}

			m_device.destroyImageView(offscreenData.alloc_images[i].image_view);
			m_allocator.destroyImage(offscreenData.alloc_images[i].image, offscreenData.alloc_images[i].allocation);
		}
	}

	void RenderSubystemVK::update_texture_descriptors()
	{
		if (m_initialized && current_frame_data().texture_descriptor_needs_updated)
		{
			std::vector<vk::DescriptorImageInfo> texture_image_infos;
			build_texture_descriptor_info(m_tex_data, texture_image_infos);

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
			                               m_global_render_data.descriptor_allocator)
				.updateImages(0, texture_image_infos.size(), texture_image_infos.data(),
				              vk::DescriptorType::eCombinedImageSampler)
				.update(current_frame_data().texture_descriptor);

			current_frame_data().texture_descriptor_needs_updated = false;
		}
	}

	void RenderSubystemVK::update_shadow_descriptors()
	{
		if (m_initialized && current_frame_data().shadow_descriptor_needs_updated)
		{
			std::vector<vk::DescriptorImageInfo> shadow_image_infos;
			build_shadow_descriptor_info(shadow_image_infos);

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
				m_global_render_data.descriptor_allocator)
				.updateImages(0, shadow_image_infos.size(), shadow_image_infos.data(),
					vk::DescriptorType::eCombinedImageSampler)
				.update(current_frame_data().shadowmap_descriptor);

			current_frame_data().shadow_descriptor_needs_updated = false;
		}
	}

	void RenderSubystemVK::prepare_scene_data()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = current_frame_data().camera_buffer;

		auto cam_system = mEngine->GetSubsystem<CameraSubystem>();
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = entt_subsystem->registry();

		auto entity = entt_subsystem->get_entity(cam_system->active_cam_id());
		auto& camera = registry->get<CameraComponent3D>(entity);

		GPUCameraData camUBO = {};
		camUBO.proj = camera.proj;
		camUBO.view = camera.view;
		camUBO.view_proj = camera.view_proj;

		memcpy(cameraBuffer.alloc_info.pMappedData, &camUBO, sizeof(GPUCameraData));

		prepare_material_data();
		prepare_object_data();
		prepare_light_data();
		prepare_shadow_data();
	}

	void RenderSubystemVK::prepare_material_data()
	{
		if (current_frame_data().copy_material_data_to_gpu)
		{
			std::vector<GPUMaterialInstanceData> materialData;
			materialData.reserve(m_material_registry->get_material_data().size());

			int idx = 0;
			for (auto& matData : m_material_registry->get_material_data())
			{
				// Update cached material data
				for (int i = 0; i < gNumTexturesPerMat; ++i)
				{
					if (matData.texIDs[i] != 0)
					{
						m_material_registry->get_cached_material_data(matData.assetId).tex_indices[i] = m_tex_data[matData.texIDs[i]].idx;
					}
				}

				materialData.push_back(m_material_registry->get_cached_material_data(matData.assetId));

				matData.idx = idx;

				idx++;
			}

			util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().material_buffer, 
			                                    materialData.size() * sizeof(GPUMaterialInstanceData), materialData.data());

			current_frame_data().copy_material_data_to_gpu = false;
		}
	}

	void RenderSubystemVK::prepare_object_data()
	{
		if (!m_objects_to_refresh.empty())
		{
			const auto enkiTSSubSystem = mEngine->GetSubsystem<core::EnkiTSSubsystem>();

			// Calculate t value for rendering interpolated position
			const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

			std::vector<PuffinID> objectsToRefresh;
			objectsToRefresh.reserve(m_objects_to_refresh.size());

			for (const auto id : m_objects_to_refresh)
			{
				objectsToRefresh.push_back(id);
			}

			const auto numObjectsToRefresh = objectsToRefresh.size();

			const uint32_t numThreads = enkiTSSubSystem->get_task_scheduler()->GetNumTaskThreads();

			// Temp object vectors for writing to by threads
			std::vector<std::vector<std::pair<PuffinID, GPUObjectData>>> threadObjects;

			threadObjects.resize(numThreads);
			for (auto& threadObject : threadObjects)
			{
				threadObject.reserve(std::ceil(g_max_objects / numThreads));
			}

			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto scene_graph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();
			const auto registry = enttSubsystem->registry();

			enki::TaskSet task(numObjectsToRefresh, [&](enki::TaskSetPartition range, uint32_t threadnum)
			{
				for (uint32_t objectIdx = range.start; objectIdx < range.end; objectIdx++)
				{
					const auto entityID = objectsToRefresh[objectIdx];
					const auto entity = enttSubsystem->get_entity(entityID);
					const auto node = scene_graph->get_node_ptr(entityID);

                    TransformComponent3D tempTransform;

					// Convert 2D transform to 3D for rendering
					if (registry->any_of<TransformComponent2D>(entity))
					{
                        TransformComponent2D* transform;

						if (node)
							transform = node->global_transform_2d();
						else
                            transform = &registry->get<TransformComponent2D>(entity);

                        tempTransform.position.x = transform->position.x;
                        tempTransform.position.y = transform->position.y;
						tempTransform.position.z = 0.0;

                        tempTransform.orientation_quat = angleAxis(glm::radians(transform->rotation), glm::vec3(0.0f, 0.0f, 1.0f));

                        tempTransform.scale.x = transform->scale.x;
                        tempTransform.scale.y = transform->scale.y;
						tempTransform.scale.z = 1.0f;
					}
					else
					{
						if (node)
						{
                            tempTransform = *node->global_transform_3d();
						}
						else
						{
							tempTransform = registry->get<TransformComponent3D>(entity);
						}
					}

					
					const auto& mesh = registry->get<MeshComponent>(entity);

#ifdef PFN_DOUBLE_PRECISION
					Vector3d position = { 0.0 };
#else
					Vector3f position = { 0.0f };
#endif

					if (registry->any_of<physics::VelocityComponent2D, physics::VelocityComponent3D>(entity))
					{
                        physics::VelocityComponent3D velocity = physics::VelocityComponent3D();

						if (registry->any_of<physics::VelocityComponent2D>(entity))
						{
							const auto& velocity2D = registry->get<physics::VelocityComponent2D>(entity);

							velocity.linear.x = velocity2D.linear.x;
							velocity.linear.y = velocity2D.linear.y;
							velocity.angular.z = velocity2D.angular;
						}
						else
						{
							velocity = registry->get<physics::VelocityComponent3D>(entity);
						}

#ifdef PFN_DOUBLE_PRECISION
						Vector3d interpolatedPosition = tempTransform.position + velocity.linear * mEngine->timeStepFixed();
#else
						Vector3f interpolatedPosition = tempTransform.position + velocity.linear * mEngine->GetTimeStepFixed();
#endif

						position = maths::lerp(tempTransform.position, interpolatedPosition, t);
					}
					else
					{
						position = tempTransform.position;
					}

					GPUObjectData object;

					build_model_transform(position, tempTransform.orientation_quat, tempTransform.scale, object.model);
					object.mat_idx = m_material_registry->get_material_data(mesh.mat_asset_id).idx;

					threadObjects[threadnum].emplace_back(entityID, object);
				}
			});

			task.m_MinRange = 500; // Try and ensure each thread gets a minimum of transforms matrices to calculate

			enkiTSSubSystem->get_task_scheduler()->AddTaskSetToPipe(&task);

			enkiTSSubSystem->get_task_scheduler()->WaitforTask(&task);

			for (const auto& tempThreadObjects : threadObjects)
			{
				for (const auto& [idx, object] : tempThreadObjects)
				{
					if (m_cached_object_data.contains(idx))
					{
						m_cached_object_data[idx] = object;
					}
				}
			}

			m_objects_to_refresh.clear();
		}

		if (current_frame_data().copy_object_data_to_gpu)
		{
			std::vector<GPUObjectData> objects = {};
			objects.reserve(g_max_objects);

			for (const auto& renderable : m_renderables)
			{
				objects.emplace_back(m_cached_object_data[renderable.entityID]);
			}

			util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().object_buffer,
			                                    objects.size() * sizeof(GPUObjectData), objects.data());

			current_frame_data().copy_object_data_to_gpu = false;
		}
	}

	void RenderSubystemVK::prepare_light_data()
	{
		// Prepare dynamic light data
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		const auto light_view = registry->view<const TransformComponent3D, LightComponent>();

		std::vector<GPULightData> lights;

		int i = 0;

		for (auto [entity, transform, light] : light_view.each())
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= g_max_lights)
			{
				break;
			}

			lights.emplace_back();

			lights[i].position_and_type.x = transform.position.x;
			lights[i].position_and_type.y = transform.position.y;
			lights[i].position_and_type.z = transform.position.z;
			lights[i].position_and_type.w = static_cast<int>(light.type);

			glm::vec4 dir = { 0.5f, -0.5f, 0.0f, 1.0f };

			dir = glm::rotateZ(dir, maths::deg_to_rad(transform.orientation_euler_angles.roll));
			dir = glm::rotateX(dir, maths::deg_to_rad(transform.orientation_euler_angles.pitch));
			dir = glm::rotateY(dir, maths::deg_to_rad(transform.orientation_euler_angles.yaw));

			dir = glm::normalize(dir);

			light.direction = { dir.x, dir.y, dir.z };

			lights[i].direction.x = light.direction.x;
			lights[i].direction.y = light.direction.y;
			lights[i].direction.z = light.direction.z;
			lights[i].direction.w = 0.0f;

			lights[i].color.x = light.color.x;
			lights[i].color.y = light.color.y;
			lights[i].color.z = light.color.z;
			lights[i].color.w = 0.0f;

			lights[i].ambient_specular.x = light.ambient_intensity;
			lights[i].ambient_specular.y = light.specular_intensity;
			lights[i].ambient_specular.z = light.specular_exponent;
			lights[i].ambient_specular.w = 0.0f;

			lights[i].attenuation.x = light.constant_attenuation;
			lights[i].attenuation.y = light.linear_attenuation;
			lights[i].attenuation.z = light.quadratic_attenuation;
			lights[i].attenuation.w = 0.0f;

			lights[i].cutoff_angle_and_shadow_index.x = glm::cos(glm::radians(light.inner_cutoff_angle));
			lights[i].cutoff_angle_and_shadow_index.y = glm::cos(glm::radians(light.outer_cutoff_angle));
			lights[i].cutoff_angle_and_shadow_index.z = -1.0f;
			lights[i].cutoff_angle_and_shadow_index.w = 0.0f;

			if (registry->any_of<ShadowCasterComponent>(entity))
			{
				auto& shadow = registry->get<ShadowCasterComponent>(entity);

				if (light.type != LightType::Point)
				{
					lights[i].cutoff_angle_and_shadow_index.z = shadow.shadow_idx;
				}
			}

			i++;
		}

		// Copy light data to buffer
		util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().light_buffer,
		                                    lights.size() * sizeof(GPULightData), lights.data());

		auto cam_system = mEngine->GetSubsystem<CameraSubystem>();
		auto entity = entt_subsystem->get_entity(cam_system->active_cam_id());
		auto& transform = registry->get<TransformComponent3D>(entity);

		// Prepare light static data
		current_frame_data().push_constant_frag.view_pos_and_light_count.x = transform.position.x;
		current_frame_data().push_constant_frag.view_pos_and_light_count.y = transform.position.y;
		current_frame_data().push_constant_frag.view_pos_and_light_count.z = transform.position.z;
		current_frame_data().push_constant_frag.view_pos_and_light_count.w = i;
	}

	void RenderSubystemVK::prepare_shadow_data()
	{
		// Prepare dynamic light data
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		const auto shadow_view = registry->view<const TransformComponent3D, const LightComponent, ShadowCasterComponent>();

		m_shadows_to_draw.clear();

		std::vector<GPUShadowData> shadows;
        std::vector<GPUShadowCascadeData> shadow_cascades;

        shadows.reserve(g_max_lights);
        shadow_cascades.reserve(g_max_lights * g_max_shadow_cascades_per_light);

		int i = 0;
        int c = 0;

		for (auto [entity, transform, light, shadow] : shadow_view.each())
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= g_max_lights)
			{
				break;
			}

			if (light.type == LightType::Spot || light.type == LightType::Directional)
			{
				shadows.emplace_back();

				if (light.type == LightType::Spot)
				{
					float near_plane = 1.f;
					float far_plane = 100.f;
					float aspect = float(shadow.width) / float(shadow.height);

					glm::mat4 light_view = glm::lookAt(static_cast<glm::vec3>(transform.position), static_cast<glm::vec3>(transform.position + light.direction), glm::vec3(0, 1, 0));

					glm::mat4 light_projection = glm::perspective(glm::radians(light.outer_cutoff_angle * 2), aspect, near_plane, far_plane);
					light_projection[1][1] *= -1;

					shadow.light_view_proj = light_projection * light_view;

					shadows[i].shadow_bias.x = shadow.bias_min;
					shadows[i].shadow_bias.y = shadow.bias_max;

                    shadow_cascades.emplace_back();
                    shadow_cascades[c].light_space_view = shadow.light_view_proj;
                    shadow_cascades[c].cascade_plane_distance = 100.0f;

                    ++c;

					m_shadows_to_draw.push_back(entt_subsystem->get_id(entity));
				}
				else if (light.type == LightType::Directional)
				{
					auto cam_system = mEngine->GetSubsystem<CameraSubystem>();
					auto active_cam_entity = entt_subsystem->get_entity(cam_system->active_cam_id());
					auto& camera = registry->get<CameraComponent3D>(active_cam_entity);

					// Calculate camera view frustum vertices
					std::vector<glm::vec4> camera_frustum_vertices;
					util::calculate_camera_frustum(camera_frustum_vertices, camera.view, camera.proj);

					// Transform to light view space
					std::vector<glm::vec4> cam_light_view_vertices;
					for (const auto& v : camera_frustum_vertices)
					{
						cam_light_view_vertices.push_back(shadow.light_view * v);
					}

					// Calculate cam frustum aabb in light space
					const auto cam_light_view_aabb = util::calculate_aabb_from_vertices(cam_light_view_vertices);

					// Recalculate shadow bounds if cam aabb is not fully enclosed by shadow bounds aabb
					if (!util::check_aabb_is_enclosed(cam_light_view_aabb, shadow.bounds_aabb))
					{
						// Average vertices to get centre of view frustum
						Vector3f centre;
						for (const auto& v : camera_frustum_vertices)
						{
							centre += glm::vec3(v);
						}
						centre /= camera_frustum_vertices.size();

						// Calculate light view
						shadow.light_view = glm::lookAt(static_cast<glm::vec3>(centre - light.direction), static_cast<glm::vec3>(centre), glm::vec3(0, 1, 0));

						// Calculate light projection - ortho
						cam_light_view_vertices.clear();
						for (const auto& v : camera_frustum_vertices)
						{
							cam_light_view_vertices.push_back(shadow.light_view * v);
						}

						shadow.bounds_aabb = util::calculate_aabb_from_vertices(cam_light_view_vertices, shadow.bounds_mult);

						glm::mat4 light_projection = glm::ortho(shadow.bounds_aabb.min.x, shadow.bounds_aabb.max.x,
							shadow.bounds_aabb.min.y, shadow.bounds_aabb.max.y, shadow.bounds_aabb.min.z, shadow.bounds_aabb.max.z);
						light_projection[1][1] *= -1;

						shadow.light_view_proj = light_projection * shadow.light_view;
					}

					shadows[i].shadow_bias.x = shadow.bias_min;
					shadows[i].shadow_bias.y = shadow.bias_max;

                    shadow_cascades.emplace_back();
                    shadow_cascades[c].light_space_view = shadow.light_view_proj;
                    shadow_cascades[c].cascade_plane_distance = shadow.bounds_aabb.max.x;

                    ++c;

					m_shadows_to_draw.push_back(entt_subsystem->get_id(entity));
				}

				++i;
			}
		}

		// Copy light data to buffer
		util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().shadow_buffer,
			shadows.size() * sizeof(GPUShadowData), shadows.data());

        util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().shadow_cascade_buffer,
            shadow_cascades.size() * sizeof(GPUShadowCascadeData), shadow_cascades.data());
	}

	void RenderSubystemVK::build_indirect_commands()
	{
		if (!m_renderables.empty())
		{
			std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
			indirectCmds.resize(g_max_objects);

			m_draw_batches.clear();
			m_draw_batches.reserve(m_material_registry->get_material_data().size());

			bool newBatch = false;
			int cmdIdx = 0;
			int cmdCount = 0;
			int instanceIdx = 0;
			int instanceCount = 0;
			PuffinID currentMeshID = m_renderables[0].meshID;
			uint8_t currentSubMeshIdx = m_renderables[0].subMeshIdx;

			MeshDrawBatch drawBatch;
			drawBatch.matID = m_renderables[0].matID;
			drawBatch.cmdIndex = 0;

			indirectCmds[cmdIdx].vertexOffset = m_resource_manager->geometry_buffer()->mesh_vertex_offset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstIndex = m_resource_manager->geometry_buffer()->mesh_index_offset(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].indexCount = m_resource_manager->geometry_buffer()->mesh_index_count(currentMeshID, currentSubMeshIdx);
			indirectCmds[cmdIdx].firstInstance = 0;

			constexpr int maxInstancesPerCommand = g_max_objects;
			constexpr int maxCommandsPerBatch = g_max_objects;

			for (const auto& [entityID, meshID, matID, subMeshIdx] : m_renderables)
			{
				// Push current draw batch struct to vector when material changes or max commands per batch is exceeded
				if (drawBatch.matID != matID || cmdCount >= maxCommandsPerBatch)
				{
					drawBatch.cmdCount = cmdCount;
					cmdCount = 0;

					m_draw_batches.push_back(drawBatch);

					drawBatch.matID = matID;
					drawBatch.cmdIndex = cmdIdx;

					newBatch = true;
				}

				// Start a new command when a new mesh is encountered, when a new batch is started or when maxInstancesPerCommand is exceeded
				if (currentMeshID != meshID || currentSubMeshIdx != subMeshIdx || newBatch || instanceCount >= maxInstancesPerCommand)
				{
					currentMeshID = meshID;
					currentSubMeshIdx = subMeshIdx;

					indirectCmds[cmdIdx].instanceCount = instanceCount;
					instanceCount = 0;

					cmdIdx++;
					cmdCount++;

					indirectCmds[cmdIdx].vertexOffset = m_resource_manager->geometry_buffer()->mesh_vertex_offset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstIndex = m_resource_manager->geometry_buffer()->mesh_index_offset(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].indexCount = m_resource_manager->geometry_buffer()->mesh_index_count(currentMeshID, currentSubMeshIdx);
					indirectCmds[cmdIdx].firstInstance = instanceIdx;

					newBatch = false;
				}

				instanceIdx++;
				instanceCount++;
			}

			// Fill out last command
			indirectCmds[cmdIdx].instanceCount = instanceCount;

			cmdCount++;

			drawBatch.cmdCount = cmdCount;

			// Push final draw batch struct to vector at end of loop
			m_draw_batches.push_back(drawBatch);

			util::copy_cpu_data_into_gpu_buffer(this, current_frame_data().indirect_buffer,
			                                    indirectCmds.size() * sizeof(vk::DrawIndexedIndirectCommand), indirectCmds.data());
		}
	}

	vk::CommandBuffer& RenderSubystemVK::record_shadow_command_buffer(uint32_t swapchain_idx)
	{
		auto& cmd = current_frame_data().shadow_command_buffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_shadow_pipeline.get());

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_shadow_pipeline_layout.get(), 0, 1,
			&current_frame_data().object_descriptor, 0, nullptr);

		cmd.bindIndexBuffer(m_resource_manager->geometry_buffer()->index_buffer().buffer, 0, vk::IndexType::eUint32);

		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();

		for (auto id : m_shadows_to_draw)
		{
			const auto& entity = entt_subsystem->get_entity(id);
			const auto& shadow = entt_subsystem->registry()->get<ShadowCasterComponent>(entity);

			GPUShadowPushConstant push_constant;
			push_constant.vertex_buffer_address = m_resource_manager->geometry_buffer()->vertex_buffer_address();
			push_constant.light_space_view = shadow.light_view_proj;

			cmd.pushConstants(m_shadow_pipeline_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUShadowPushConstant), &push_constant);

			draw_shadowmap(cmd, m_resource_manager->get_image(shadow.resource_id, current_frame_idx()), { shadow.width, shadow.height });
		}

		cmd.end();

		return cmd;
	}

	void RenderSubystemVK::draw_shadowmap(vk::CommandBuffer cmd, const AllocatedImage& depth_image, const vk::Extent2D& shadow_extent)
	{
		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_depth = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, {}, {},
			depth_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eLateFragmentTests,
			{}, 0, nullptr, 0, nullptr,
			1, & offscreen_memory_barrier_to_depth);

		vk::ClearValue depth_clear;
		depth_clear.depthStencil.depth = 1.f;

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			depth_image.image_view, vk::ImageLayout::eDepthAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depth_clear
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, shadow_extent}, 1, {}, 0, {}, &depth_attach_info
		};

		cmd.beginRendering(&render_info);

		set_draw_parameters(cmd, shadow_extent);

		// Make a indirect draw call for each material
		for (const auto& draw_batch : m_draw_batches)
		{
			draw_mesh_batch(cmd, draw_batch);
		}

		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			depth_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_shader);
	}

	vk::CommandBuffer& RenderSubystemVK::record_main_command_buffer(const uint32_t& swapchain_idx,
	                                                              const vk::Extent2D& render_extent,
	                                                              const AllocatedImage&
	                                                              color_image, const AllocatedImage& depth_image)
	{
		auto& cmd = current_frame_data().main_command_buffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_color = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			color_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier_to_color);

		vk::ClearValue color_clear;
		color_clear.color = {0.0f, 0.7f, 0.9f, 1.0f};

		vk::ClearValue depth_clear;
		depth_clear.depthStencil.depth = 1.f;

		// Begin Rendering
		vk::RenderingAttachmentInfoKHR color_attach_info = {
			color_image.image_view, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, color_clear
		};

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			depth_image.image_view, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depth_clear
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, render_extent}, 1, {}, 1, &color_attach_info, &depth_attach_info
		};

		cmd.beginRendering(&render_info);

		draw_objects(cmd, render_extent);

		// End Rendering
		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			color_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier_to_shader);

		// Finish command buffer recording
		cmd.end();

		return cmd;
	}

	void RenderSubystemVK::draw_objects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		set_draw_parameters(cmd, renderExtent);

		bind_buffers_and_descriptors(cmd);

		// Make a indirect draw call for each material
		for (const auto& draw_batch : m_draw_batches)
		{
			// Use loaded material if id is valid, otherwise use default material
			if (draw_batch.matID != gInvalidID)
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_material_registry->get_material(draw_batch.matID).pipeline.get());
			}
			else
			{
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_forward_pipeline.get());
			}

			draw_mesh_batch(cmd, draw_batch);
		}
	}

	void RenderSubystemVK::set_draw_parameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { {0, 0}, {renderExtent.width, renderExtent.height} };
		cmd.setScissor(0, 1, &scissor);
	}

	void RenderSubystemVK::bind_buffers_and_descriptors(vk::CommandBuffer cmd)
	{
		std::vector<vk::DescriptorSet> descriptors;
		descriptors.push_back(current_frame_data().object_descriptor);
		descriptors.push_back(current_frame_data().global_descriptor);
		descriptors.push_back(current_frame_data().texture_descriptor);
		descriptors.push_back(current_frame_data().shadowmap_descriptor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_forward_pipeline_layout.get(), 0, descriptors.size(),
			descriptors.data(), 0, nullptr);

		
		GPUVertexShaderPushConstant push_constant_vert;
		push_constant_vert.vertex_buffer_address = m_resource_manager->geometry_buffer()->vertex_buffer_address();

		cmd.pushConstants(m_forward_pipeline_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUVertexShaderPushConstant), &push_constant_vert);
		cmd.pushConstants(m_forward_pipeline_layout.get(), vk::ShaderStageFlagBits::eFragment, sizeof(GPUVertexShaderPushConstant), sizeof(GPUVertexShaderPushConstant), &current_frame_data().push_constant_frag);
		
		cmd.bindIndexBuffer(m_resource_manager->geometry_buffer()->index_buffer().buffer, 0, vk::IndexType::eUint32);
	}

	void RenderSubystemVK::draw_mesh_batch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch)
	{
		vk::DeviceSize indirect_offset = meshDrawBatch.cmdIndex * sizeof(vk::DrawIndexedIndirectCommand);
		uint32_t draw_stride = sizeof(vk::DrawIndexedIndirectCommand);

		draw_indexed_indirect_command(cmd, current_frame_data().indirect_buffer.buffer, indirect_offset,
			meshDrawBatch.cmdCount, draw_stride);
	}

	void RenderSubystemVK::draw_indexed_indirect_command(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer,
	                                                vk::DeviceSize offset,
	                                                uint32_t drawCount, uint32_t stride)
	{
		cmd.drawIndexedIndirect(indirectBuffer, offset, drawCount, stride);
		m_draw_calls++;
	}

	vk::CommandBuffer& RenderSubystemVK::record_copy_command_buffer(uint32_t swapchainIdx)
	{
		auto& cmd = current_frame_data().copy_command_buffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Setup pipeline barriers for transitioning image layouts

		vk::ImageSubresourceRange image_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		// Offscreen Transition
		vk::ImageMemoryBarrier offscreen_memory_barrier = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal, {}, {},
			m_offscreen_data.alloc_images[swapchainIdx].image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier);

		// Swapchain Transition
		vk::ImageMemoryBarrier swapchain_memory_barrier = {
			vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {}, {},
			m_swapchain_data.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchain_memory_barrier);

		// Blit (Copy with auto format coversion (RGB to BGR)) offscreen to swapchain image
		vk::Offset3D blit_size =
		{
			static_cast<int32_t>(m_offscreen_data.extent.width),
			static_cast<int32_t>(m_offscreen_data.extent.height),
			1
		};

		std::array<vk::Offset3D, 2> offsets = {};
		offsets[1] = blit_size;

		vk::ImageBlit image_blit_region =
		{
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets,
			{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, offsets
		};

		cmd.blitImage(m_offscreen_data.alloc_images[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
		              m_swapchain_data.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &image_blit_region,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreen_memory_barrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			m_offscreen_data.alloc_images[swapchainIdx].image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &offscreen_memory_barrier);

		// Swapchain Transition
		swapchain_memory_barrier = {
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			m_swapchain_data.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
		                    {}, 0, nullptr, 0, nullptr,
		                    1, &swapchain_memory_barrier);

		cmd.end();

		return cmd;
	}

	vk::CommandBuffer& RenderSubystemVK::record_imgui_command_buffer(uint32_t swapchainIdx,
	                                                               const vk::Extent2D& renderExtent)
	{
		auto& cmd = current_frame_data().imgui_command_buffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_color = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, {},
			m_swapchain_data.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_color);

		vk::ClearValue clear_value;
		clear_value.color = {1.0f, 1.0f, 1.0f, 1.0f};

		// Begin rendering
		vk::RenderingAttachmentInfoKHR color_attach_info = {
			m_swapchain_data.image_views[swapchainIdx], vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clear_value
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, renderExtent}, 1, {}, 1, &color_attach_info
		};

		cmd.beginRendering(&render_info);

		// Record Imgui Draw Data and draw functions into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		cmd.endRendering();

		// Transition layout for presenting to swapchain
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, {}, {},
			m_swapchain_data.images[swapchainIdx], image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_shader);

		cmd.end();

		return cmd;
	}

	void RenderSubystemVK::record_and_submit_commands(uint32_t swapchainIdx)
	{
		std::vector<vk::SubmitInfo> submits;

		// Prepare shadow rendering command submit
		std::vector<vk::CommandBuffer*> shadow_commands = {};
		std::vector<vk::Semaphore*> shadow_wait_semaphores = { &current_frame_data().present_semaphore };
		std::vector<vk::Semaphore*> shadow_signal_semaphores = { &current_frame_data().shadow_semaphore };
		vk::PipelineStageFlags shadow_wait_stage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		if (m_render_shadows)
		{
			shadow_commands.push_back(&record_shadow_command_buffer(swapchainIdx));

			vk::SubmitInfo shadow_submit =
			{
				static_cast<uint32_t>(shadow_wait_semaphores.size()), *shadow_wait_semaphores.data(),
				& shadow_wait_stage, static_cast<uint32_t>(shadow_commands.size()), *shadow_commands.data(),
				static_cast<uint32_t>(shadow_signal_semaphores.size()), *shadow_signal_semaphores.data(), nullptr
			};

			submits.push_back(shadow_submit);
		}

		// Prepare main render command submit
		std::vector<vk::CommandBuffer*> render_commands = { };
		std::vector<vk::Semaphore*> render_wait_semaphores = { };
		std::vector<vk::PipelineStageFlags> render_wait_stages = { };
		std::vector<vk::Semaphore*> render_signal_semaphores = { &current_frame_data().render_semaphore };
		{
			render_commands.push_back(&record_main_command_buffer(swapchainIdx, m_offscreen_data.extent,
				m_offscreen_data.alloc_images[swapchainIdx],
				m_offscreen_data.alloc_depth_image));

			if (m_render_shadows)
			{
				render_wait_semaphores.push_back(&current_frame_data().shadow_semaphore);
				render_wait_stages.emplace_back(vk::PipelineStageFlagBits::eBottomOfPipe);
			}
			else
			{
				render_wait_semaphores.push_back(&current_frame_data().present_semaphore);
				render_wait_stages.emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
			}

			vk::SubmitInfo render_submit =
			{
				static_cast<uint32_t>(render_wait_semaphores.size()), *render_wait_semaphores.data(),
				render_wait_stages.data(), static_cast<uint32_t>(render_commands.size()), *render_commands.data(),
				static_cast<uint32_t>(render_signal_semaphores.size()), *render_signal_semaphores.data(), nullptr
			};

			submits.push_back(render_submit);
		}

		{
			vk::PipelineStageFlags wait_stage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

			if (mEngine->GetShouldRenderEditorUI())
			{
				auto& imgui_cmd = record_imgui_command_buffer(swapchainIdx, m_swapchain_data.extent);

				vk::SubmitInfo imgui_submit =
				{
					1, &current_frame_data().render_semaphore,
					&wait_stage, 1, &imgui_cmd,
					1, &current_frame_data().imgui_semaphore, nullptr
				};

				submits.push_back(imgui_submit);
			}
			else
			{
				auto& copy_cmd = record_copy_command_buffer(swapchainIdx);

				vk::SubmitInfo copy_submit =
				{
					1, &current_frame_data().render_semaphore,
					&wait_stage, 1, &copy_cmd,
					1, &current_frame_data().copy_semaphore, nullptr
				};

				submits.push_back(copy_submit);
			}
		}

		VK_CHECK(m_graphics_queue.submit(submits.size(), submits.data(), current_frame_data().render_fence));

		vk::Semaphore wait_semaphore;

		if (mEngine->GetShouldRenderEditorUI())
		{
			wait_semaphore = current_frame_data().imgui_semaphore;
		}
		else
		{
			wait_semaphore = current_frame_data().copy_semaphore;
		}

		vk::PresentInfoKHR present_info =
		{
			1, &wait_semaphore, 1, &m_swapchain_data.swapchain, &swapchainIdx
		};

		VK_CHECK(m_graphics_queue.presentKHR(&present_info));
	}

	void RenderSubystemVK::build_model_transform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
	                                         glm::mat4& model)
	{
		const auto scaleM = glm::scale(glm::mat4(1.0f), static_cast<glm::vec3>(scale));

		const auto orientM = glm::toMat4(static_cast<glm::quat>(orientation));

		const auto translateM = glm::translate(glm::mat4(1.0f), static_cast<glm::vec3>(position));

		model = translateM * orientM * scaleM;
	}

	/*bool VKRenderSystem::loadMesh(PuffinID meshId, MeshDataVK& meshData)
	{
		if (const auto meshAsset = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshId); meshAsset && meshAsset->load())
		{
			meshData.assetId = meshId;

			meshData.numVertices = meshAsset->numVertices();
			meshData.numIndices = meshAsset->numIndices();

			meshData.vertexBuffer = util::initVertexBuffer(this, meshAsset->vertices().data(),
			                                               meshAsset->numVertices(), meshAsset->vertexSize());

			meshData.indexBuffer = util::initIndexBuffer(this, meshAsset->indices().data(),
			                                             meshAsset->numIndices(), meshAsset->indexSize());

			meshAsset->unload();

			return true;
		}
		else
		{
			return false;
		}
	}*/

	/*void VKRenderSystem::unloadMesh(MeshDataVK& meshData) const
	{
		mAllocator.destroyBuffer(meshData.vertexBuffer.buffer, meshData.vertexBuffer.allocation);
		mAllocator.destroyBuffer(meshData.indexBuffer.buffer, meshData.indexBuffer.allocation);
	}*/

	bool RenderSubystemVK::load_texture(PuffinID texId, TextureDataVK& texData)
	{
		if (const auto texAsset = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texId); texAsset && texAsset->Load())
		{
			texData.assetId = texId;

			texData.sampler = m_global_render_data.texture_sampler;

			texData.texture = util::init_texture(this, texAsset->PixelData(),
			                                     texAsset->TextureWidth(), texAsset->TextureHeight(),
			                                     texAsset->TextureSize(),
			                                     g_tex_format_vk.at(texAsset->TextureFormat()));

			texAsset->Unload();

			return true;
		}

		return false;
	}

	void RenderSubystemVK::unload_texture(TextureDataVK& texData) const
	{
		m_device.destroyImageView(texData.texture.image_view);
		m_allocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void RenderSubystemVK::build_texture_descriptor_info(PackedVector<PuffinID, TextureDataVK>& textureData,
	                                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.size());

		int idx = 0;
		for (auto& texData : textureData)
		{
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texData.texture.image_view, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}

	void RenderSubystemVK::build_shadow_descriptor_info(std::vector<vk::DescriptorImageInfo>& shadow_image_infos)
	{
		auto entt_subsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();
		const auto shadow_view = registry->view<const TransformComponent3D, const LightComponent, ShadowCasterComponent>();

		shadow_image_infos.clear();
		shadow_image_infos.reserve(shadow_view.size_hint());

		int idx = 0;
		for (auto [entity, transform, light, shadow] : shadow_view.each())
		{
			auto id = entt_subsystem->get_id(entity);

			auto& alloc_image = m_resource_manager->get_image(shadow.resource_id, current_frame_idx());

			vk::DescriptorImageInfo shadow_image_info = { m_global_render_data.shadowmap_sampler, alloc_image.image_view,
				vk::ImageLayout::eShaderReadOnlyOptimal };

			shadow_image_infos.push_back(shadow_image_info);

			shadow.shadow_idx = idx;
			++idx;
		}
	}

	FrameRenderData& RenderSubystemVK::current_frame_data()
	{
		return m_frame_render_data[m_frame_count % g_buffered_frames];
	}

	void RenderSubystemVK::frame_buffer_resize_callback(GLFWwindow* window, const int width, const int height)
	{
		const auto system = static_cast<RenderSubystemVK*>(glfwGetWindowUserPointer(window));

		system->m_swapchain_data.resized = true;
		system->m_offscreen_data.resized = true;
		system->m_window_size.width = width;
		system->m_window_size.height = height;
	}
}
