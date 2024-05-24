#include "puffin/rendering/vulkan/render_system_vk.h"

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "puffin/math_helpers.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "glm/glm.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "puffin/core/engine.h"
#include "puffin/rendering/vulkan/helpers_vk.h"
#include "puffin/core/enkits_subsystem.h"
#include "puffin/core/signal_subsystem.h"
#include "puffin/components/transform_component_2d.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/input/input_subsystem.h"
#include "puffin/assets/asset_registry.h"
#include "puffin/components/physics/2d/velocity_component_2d.h"
#include "puffin/components/physics/3d/velocity_component_3d.h"
#include "puffin/components/rendering/light_component.h"
#include "puffin/components/rendering/mesh_component.h"
#include "puffin/rendering/material_globals.h"
#include "puffin/window/window_subsystem.h"
#include "puffin/scene/scene_graph.h"
#include "puffin/ui/editor/ui_subsystem.h"
#include "puffin/ui/editor/windows/ui_window_viewport.h"
#include "puffin/rendering/vulkan/resource_manager_vk.h"

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
	RenderSystemVK::RenderSystemVK(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::Startup, [&]() { startup(); }, "VKRenderSystem: Startup");
		mEngine->registerCallback(core::ExecutionStage::Render, [&]() { render(); }, "VKRenderSystem: Render");
		mEngine->registerCallback(core::ExecutionStage::Shutdown, [&]() { shutdown(); }, "VKRenderSystem: Shutdown");

		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		registry->on_construct<MeshComponent>().connect<&RenderSystemVK::on_update_mesh>(this);
		registry->on_update<MeshComponent>().connect<&RenderSystemVK::on_update_mesh>(this);
		registry->on_destroy<MeshComponent>().connect<&RenderSystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<TransformComponent2D>().connect<&RenderSystemVK::on_update_transform>(this);
		registry->on_update<TransformComponent2D>().connect<&RenderSystemVK::on_update_transform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&RenderSystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<TransformComponent3D>().connect<&RenderSystemVK::on_update_transform>(this);
		registry->on_update<TransformComponent3D>().connect<&RenderSystemVK::on_update_transform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&RenderSystemVK::on_destroy_mesh_or_transform>(this);

		registry->on_construct<ShadowCasterComponent>().connect<&RenderSystemVK::on_update_shadow_caster>(this);
		registry->on_update<ShadowCasterComponent>().connect<&RenderSystemVK::on_update_shadow_caster>(this);
	}

	void RenderSystemVK::startup()
	{
		init_vulkan();

		init_swapchain(m_swapchain_data, m_swapchain_data_old.swapchain, m_window_size);

		if (mEngine->shouldRenderEditorUI())
		{
			const ImVec2 viewportSize = mEngine->getSystem<ui::UISubsystem>()->windowViewport()->viewportSize();
			m_render_extent.width = viewportSize.x;
			m_render_extent.height = viewportSize.y;
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

		init_descriptors();
		init_pipelines();

		if (mEngine->shouldRenderEditorUI())
		{
			init_imgui();
			init_offscreen_imgui_textures(m_offscreen_data);
		}

		m_editor_cam.position = {0.0f, 0.0f, 10.0f};

		m_renderables.reserve(g_max_objects);
		m_cached_object_data.reserve(g_max_objects);

		m_initialized = true;

		m_update_renderables = true;

		m_resource_manager = new ResourceManagerVK(shared_from_this());
		m_material_registry.init(shared_from_this());
	}

	void RenderSystemVK::render()
	{
		update_render_data();

		process_components();

		draw();
	}

	void RenderSystemVK::shutdown()
	{
		m_device.waitIdle();

		if (m_initialized)
		{
			for (auto texData : m_tex_data)
			{
				unload_texture(texData);
			}

			m_tex_data.clear();

			delete m_resource_manager;
			m_resource_manager = nullptr;

			clean_swapchain(m_swapchain_data);

			if (m_swapchain_data_old.needsCleaned)
			{
				clean_swapchain(m_swapchain_data_old);
			}

			clean_offscreen(m_offscreen_data);

			if (m_offscreen_data_old.needsCleaned)
			{
				clean_offscreen(m_offscreen_data_old);
			}

			m_deletion_queue.flush();

			m_initialized = false;
		}
	}

	void RenderSystemVK::on_update_mesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<MeshComponent>(entity);

		if (mesh.mesh_asset_id == gInvalidID || mesh.mat_asset_id == gInvalidID)
		{
			return;
		}

		m_meshes_to_load.insert(mesh.mesh_asset_id);
		m_material_registry.registerMaterialInstance(mesh.mat_asset_id);

		add_renderable(registry, entity);
	}

	void RenderSystemVK::on_update_transform(entt::registry& registry, entt::entity entity)
	{
		add_renderable(registry, entity);
	}

	void RenderSystemVK::on_destroy_mesh_or_transform(entt::registry& registry, entt::entity entity)
	{
		m_update_renderables = true;
	}

	void RenderSystemVK::add_renderable(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<TransformComponent2D, TransformComponent3D>(entity) && registry.any_of<MeshComponent>(entity))
		{
			const auto id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);
			const auto mesh = registry.get<MeshComponent>(entity);

			if (mesh.mesh_asset_id == gInvalidID || mesh.mat_asset_id == gInvalidID)
			{
				return;
			}

			m_objects_to_refresh.insert(id);

			m_update_renderables = true;
		}
	}

	void RenderSystemVK::on_update_shadow_caster(entt::registry& registry, entt::entity entity)
	{

	}

	void RenderSystemVK::on_destroy_shadow_caster(entt::registry& registry, entt::entity entity)
	{

	}

	void RenderSystemVK::register_texture(PuffinID texID)
	{
		m_textures_to_load.insert(texID);
	}

	void RenderSystemVK::init_vulkan()
	{
		GLFWwindow* glfwWindow = mEngine->getSystem<window::WindowSubsystem>()->primaryWindow();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, frame_buffer_resize_callback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
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
		glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &surface);
		m_surface = surface;

		vk::PhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing = true;
		physicalDeviceFeatures.drawIndirectFirstInstance = true;
		physicalDeviceFeatures.multiDrawIndirect = true;

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

	void RenderSystemVK::init_swapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain,
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
		swapchainData.imageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);

		// Grab Images
		std::vector<VkImage> images = vkbSwapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();

		swapchainData.images.resize(images.size());
		swapchainData.imageViews.resize(imageViews.size());

		for (int i = 0; i < images.size(); i++)
		{
			swapchainData.images[i] = static_cast<vk::Image>(images[i]);
			swapchainData.imageViews[i] = static_cast<vk::ImageView>(imageViews[i]);
		}

		images.clear();
		imageViews.clear();
	}

	void RenderSystemVK::init_offscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent,
	                                   const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = {offscreenData.extent.width, offscreenData.extent.height, 1};

		offscreenData.imageFormat = vk::Format::eR8G8B8A8Unorm;

		const vk::ImageCreateInfo imageInfo = {
			{}, vk::ImageType::e2D, offscreenData.imageFormat, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
			}
		};

		const vk::ImageSubresourceRange subresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

		const vk::ImageViewCreateInfo imageViewInfo = {
			{}, {}, vk::ImageViewType::e2D, offscreenData.imageFormat, {}, subresourceRange
		};

		offscreenData.allocImages.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.allocImages[i] = util::create_image(shared_from_this(), imageInfo, imageViewInfo);
		}

		offscreenData.allocDepthImage = util::init_depth_image(shared_from_this(), imageExtent, vk::Format::eD32Sfloat);
	}

	void RenderSystemVK::init_commands()
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

	void RenderSystemVK::init_sync_structures()
	{
		vk::FenceCreateInfo fenceCreateInfo = {vk::FenceCreateFlagBits::eSignaled, nullptr};
		vk::SemaphoreCreateInfo semaphoreCreateInfo = {{}, nullptr};

		for (int i = 0; i < g_buffered_frames; i++)
		{
			VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_frame_render_data[i].render_fence));

			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].render_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].copy_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].imgui_semaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frame_render_data[i].present_semaphore));

			m_deletion_queue.pushFunction([=]()
			{
				m_device.destroyFence(m_frame_render_data[i].render_fence);

				m_device.destroySemaphore(m_frame_render_data[i].render_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].copy_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].imgui_semaphore);
				m_device.destroySemaphore(m_frame_render_data[i].present_semaphore);
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

	void RenderSystemVK::init_buffers()
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

			m_frame_render_data[i].light_buffer = util::create_buffer(m_allocator, sizeof(GPULightData) * g_max_lights_vk,
			                                                     vk::BufferUsageFlagBits::eStorageBuffer,
			                                                     vma::MemoryUsage::eAuto,
			                                                     {
				                                                     vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
				                                                     | vma::AllocationCreateFlagBits::eMapped
			                                                     });

			m_frame_render_data[i].light_static_buffer = util::create_buffer(m_allocator, sizeof(GPULightStaticData),
			                                                           vk::BufferUsageFlagBits::eUniformBuffer,
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
				m_allocator.destroyBuffer(m_frame_render_data[i].light_static_buffer.buffer,
				                         m_frame_render_data[i].light_static_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].light_buffer.buffer,
				                         m_frame_render_data[i].light_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].camera_buffer.buffer,
				                         m_frame_render_data[i].camera_buffer.allocation);
				m_allocator.destroyBuffer(m_frame_render_data[i].indirect_buffer.buffer,
				                         m_frame_render_data[i].indirect_buffer.allocation);
			});
		}
	}

	void RenderSystemVK::init_samplers()
	{
		const vk::SamplerCreateInfo samplerInfo = {};

		m_global_render_data.texture_sampler = m_device.createSampler(samplerInfo);

		m_deletion_queue.pushFunction([=]()
		{
			m_device.destroySampler(m_global_render_data.texture_sampler, nullptr);
		});
	}

	void RenderSystemVK::init_descriptors()
	{
		// Descriptor Allocator/Cache

		m_global_render_data.descriptor_allocator = std::make_shared<util::DescriptorAllocator>(m_device);
		m_global_render_data.descriptor_layout_cache = std::make_shared<util::DescriptorLayoutCache>(m_device);

		for (int i = 0; i < g_buffered_frames; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo cameraBufferInfo = {
				m_frame_render_data[i].camera_buffer.buffer, 0, sizeof(GPUCameraData)
			};
			vk::DescriptorBufferInfo objectBufferInfo = {
				m_frame_render_data[i].object_buffer.buffer, 0, sizeof(GPUObjectData) * g_max_objects
			};
			vk::DescriptorBufferInfo lightBufferInfo = {
				m_frame_render_data[i].light_buffer.buffer, 0, sizeof(GPULightData) * g_max_lights_vk
			};
			vk::DescriptorBufferInfo lightStaticBufferInfo = {
				m_frame_render_data[i].light_static_buffer.buffer, 0, sizeof(GPULightStaticData)
			};
			vk::DescriptorBufferInfo materialBufferInfo = {
				m_frame_render_data[i].material_buffer.buffer, 0, sizeof(GPUMaterialInstanceData) * g_max_materials
			};

			uint32_t variableDescCounts = { 128 };

			constexpr vk::DescriptorBindingFlags descriptorBindingFlags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
			vk::DescriptorSetVariableDescriptorCountAllocateInfo descriptorSetVariableDescriptorCountAllocateInfo = { 1, &variableDescCounts };

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
			                               m_global_render_data.descriptor_allocator)
				.bindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(1, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.bindBuffer(2, &lightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(3, &lightStaticBufferInfo, vk::DescriptorType::eUniformBuffer,vk::ShaderStageFlagBits::eFragment)
				.bindBuffer(4, &materialBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.bindImagesWithoutWrite(5, 128, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.addPNext(&descriptorSetVariableDescriptorCountAllocateInfo)
				.build(m_frame_render_data[i].global_descriptor, m_global_render_data.global_set_layout);

			// Material Descriptors
		}

		m_deletion_queue.pushFunction([=]()
		{
			m_global_render_data.descriptor_layout_cache = nullptr;
			m_global_render_data.descriptor_allocator = nullptr;
		});
	}

	void RenderSystemVK::init_pipelines()
	{
		build_forward_renderer_pipeline();
	}

	void RenderSystemVK::build_forward_renderer_pipeline()
	{
		m_forward_vert_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::get()->engineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_vs.spv").string()
		};

		m_forward_frag_mod = util::ShaderModule{
			m_device, fs::path(assets::AssetRegistry::get()->engineRoot() / "bin" / "vulkan" / "forward_shading" / "forward_shading_fs.spv").string()
		};

		vk::PushConstantRange range = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUDrawPushConstant) };

		util::PipelineLayoutBuilder plb{};
		m_forward_pipeline_layout = plb
		.descriptorSetLayout(m_global_render_data.global_set_layout)
		.pushConstantRange(range)
		.createUnique(m_device);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
			{}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f
		};

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = {
			0, m_offscreen_data.imageFormat, m_offscreen_data.allocDepthImage.format
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
			m_forward_pipeline_layout = {};
			m_forward_pipeline = {};
		});
	}

	void RenderSystemVK::init_imgui()
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
		GLFWwindow* glfwWindow = mEngine->getSystem<window::WindowSubsystem>()->primaryWindow();
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		std::array<VkFormat,1 > formats = { static_cast<VkFormat>(m_swapchain_data.imageFormat) };

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
		util::immediate_submit(shared_from_this(), [=](vk::CommandBuffer cmd)
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

	void RenderSystemVK::init_offscreen_imgui_textures(OffscreenData& offscreenData)
	{
		offscreenData.viewportTextures.resize(offscreenData.allocImages.size());

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			offscreenData.viewportTextures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				m_global_render_data.texture_sampler,
				offscreenData.allocImages[i].imageView,
				static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void RenderSystemVK::process_components()
	{
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		if (m_update_renderables)
		{
			const auto meshView2D = registry->view<const TransformComponent2D, const MeshComponent>();
			const auto meshView3D = registry->view<const TransformComponent3D, const MeshComponent>();

			m_renderables.clear();

			// Iterate 2D objects
			for (auto [entity, transform, mesh] : meshView2D.each())
			{
				const auto node_id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

				if (mesh.mat_asset_id == gInvalidID || mesh.mesh_asset_id == gInvalidID)
				{
					continue;
				}

				const auto& matData = m_material_registry.getMaterialData(mesh.mat_asset_id);

				m_renderables.emplace_back(node_id, mesh.mesh_asset_id, matData.baseMaterialID, mesh.sub_mesh_idx);

				if (!m_cached_object_data.contains(node_id))
				{
					m_cached_object_data.emplace(node_id, GPUObjectData());
				}
			}

			// Iterate 3D objects
			for (auto [entity, transform, mesh] : meshView3D.each())
			{
				const auto node_id = mEngine->getSystem<ecs::EnTTSubsystem>()->get_id(entity);

				if (mesh.mat_asset_id == gInvalidID || mesh.mesh_asset_id == gInvalidID)
				{
					continue;
				}

				const auto& matData = m_material_registry.getMaterialData(mesh.mat_asset_id);

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

	void RenderSystemVK::update_render_data()
	{
		// Load Meshes
		for (const auto meshID : m_meshes_to_load)
		{
			if (const auto staticMesh = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshID))
			{
				m_resource_manager->add_static_mesh(staticMesh);
			}
		}

		m_meshes_to_load.clear();

		// Load Materials
		m_material_registry.update();

		if (m_material_registry.materialDataNeedsUploaded())
		{
			for (uint32_t i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].copy_material_data_to_gpu = true;
			}
		}

		// Load Textures

		bool textureDescriptorNeedsUpdated = false;
		for (const auto texID : m_textures_to_load)
		{
			if (texID != gInvalidID && !m_tex_data.contains(texID))
			{
				TextureDataVK texData;
				load_texture(texID, texData);

				m_tex_data.emplace(texID, texData);

				textureDescriptorNeedsUpdated = true;
			}
		}

		m_textures_to_load.clear();

		if (textureDescriptorNeedsUpdated == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].texture_descriptor_needs_updated = true;
			}
		}
	}

	void RenderSystemVK::draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &current_frame_data().render_fence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &current_frame_data().render_fence));

		if (mEngine->shouldRenderEditorUI())
		{
			const ImVec2 viewportSize = mEngine->getSystem<ui::UISubsystem>()->windowViewport()->viewportSize();

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

		update_cameras();

		recreate_swapchain();
		recreate_offscreen();

		uint32_t swapchainImageIdx;
		VK_CHECK(
			m_device.acquireNextImageKHR(m_swapchain_data.swapchain, 1000000000, current_frame_data().present_semaphore,
				nullptr, &swapchainImageIdx));

		if (mEngine->shouldRenderEditorUI())
		{
			mEngine->getSystem<ui::UISubsystem>()->windowViewport()->draw(m_offscreen_data.viewportTextures[swapchainImageIdx]);

			ImGui::Render();
		}

		m_draw_calls = 0;

		// Prepare textures, scene data & indirect commands for rendering
		update_texture_descriptors();
		prepare_scene_data();
		build_indirect_commands();

		record_and_submit_commands(swapchainImageIdx);

		m_frame_number++;
	}

	void RenderSystemVK::recreate_swapchain()
	{
		// Recreate swapchain when window is resized
		if (m_swapchain_data.resized == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].swapchain_needs_updated = true;
			}

			m_swapchain_data_old = m_swapchain_data;
			m_swapchain_data_old.needsCleaned = true;

			init_swapchain(m_swapchain_data, m_swapchain_data_old.swapchain, m_window_size);

			m_swapchain_data.resized = false;
		}

		if (m_swapchain_data_old.needsCleaned == true)
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

				m_swapchain_data_old.needsCleaned = false;
			}
		}
	}

	void RenderSystemVK::clean_swapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.imageViews.size(); i++)
		{
			m_device.destroyImageView(swapchainData.imageViews[i]);
		}

		m_device.destroySwapchainKHR(swapchainData.swapchain);
	}

	void RenderSystemVK::recreate_offscreen()
	{
		if (m_offscreen_data.resized == true)
		{
			for (int i = 0; i < g_buffered_frames; i++)
			{
				m_frame_render_data[i].offscreen_needs_updated = true;
			}

			m_offscreen_data_old = m_offscreen_data;
			m_offscreen_data_old.needsCleaned = true;

			init_offscreen(m_offscreen_data, m_render_extent, m_swapchain_data.images.size());

			if (mEngine->shouldRenderEditorUI())
			{
				init_offscreen_imgui_textures(m_offscreen_data);
			}

			m_offscreen_data.resized = false;
		}

		if (m_offscreen_data_old.needsCleaned == true)
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

				m_offscreen_data_old.needsCleaned = false;
			}
		}
	}

	void RenderSystemVK::clean_offscreen(OffscreenData& offscreenData)
	{
		m_device.destroyImageView(offscreenData.allocDepthImage.imageView);
		m_allocator.destroyImage(offscreenData.allocDepthImage.image, offscreenData.allocDepthImage.allocation);

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			if (mEngine->shouldRenderEditorUI())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewportTextures[i]));
			}

			m_device.destroyImageView(offscreenData.allocImages[i].imageView);
			m_allocator.destroyImage(offscreenData.allocImages[i].image, offscreenData.allocImages[i].allocation);
		}
	}

	void RenderSystemVK::update_cameras()
	{
		update_editor_camera();

		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();
		const auto cameraView = registry->view<const TransformComponent3D, CameraComponent>();

		for (auto [entity, transform, camera] : cameraView.each())
		{
			update_camera_component(transform, camera);
		}
	}

	void RenderSystemVK::update_editor_camera()
	{
		const auto inputSubsystem = mEngine->getSystem<input::InputSubsystem>();

		if (inputSubsystem->isCursorLocked())
		{
			// Camera Movement
			if (inputSubsystem->pressed("CamMoveLeft") && !inputSubsystem->pressed("CamMoveRight"))
			{
				m_editor_cam.position += m_editor_cam.right * m_editor_cam.speed * mEngine->deltaTime();
			}

			if (inputSubsystem->pressed("CamMoveRight") && !inputSubsystem->pressed("CamMoveLeft"))
			{
				m_editor_cam.position -= m_editor_cam.right * m_editor_cam.speed * mEngine->deltaTime();
			}

			if (inputSubsystem->pressed("CamMoveForward") && !inputSubsystem->pressed("CamMoveBackward"))
			{
				m_editor_cam.position += m_editor_cam.direction * m_editor_cam.speed * mEngine->deltaTime();
			}

			if (inputSubsystem->pressed("CamMoveBackward") && !inputSubsystem->pressed("CamMoveForward"))
			{
				m_editor_cam.position -= m_editor_cam.direction * m_editor_cam.speed * mEngine->deltaTime();
			}

			if (inputSubsystem->pressed("CamMoveUp") && !inputSubsystem->pressed("CamMoveDown"))
			{
				m_editor_cam.position += m_editor_cam.up * m_editor_cam.speed * mEngine->deltaTime();
			}

			if (inputSubsystem->pressed("CamMoveDown") && !inputSubsystem->pressed("CamMoveUp"))
			{
				m_editor_cam.position -= m_editor_cam.up * m_editor_cam.speed * mEngine->deltaTime();
			}

			// Mouse Rotation
			m_editor_cam.yaw += inputSubsystem->getMouseXOffset();
			m_editor_cam.pitch -= inputSubsystem->getMouseYOffset();

			if (m_editor_cam.pitch > 89.0f)
				m_editor_cam.pitch = 89.0f;

			if (m_editor_cam.pitch < -89.0f)
				m_editor_cam.pitch = -89.0f;

			// Calculate Direction vector from yaw and pitch of camera
			m_editor_cam.direction.x = cos(maths::degToRad(m_editor_cam.yaw)) * cos(
				maths::degToRad(m_editor_cam.pitch));
			m_editor_cam.direction.y = sin(maths::degToRad(m_editor_cam.pitch));
			m_editor_cam.direction.z = sin(maths::degToRad(m_editor_cam.yaw)) * cos(
				maths::degToRad(m_editor_cam.pitch));

			m_editor_cam.direction.normalize();
		}

		// Calculate Right, Up and LookAt vectors
		m_editor_cam.right = m_editor_cam.up.cross(m_editor_cam.direction).normalized();
		m_editor_cam.look_at = m_editor_cam.position + m_editor_cam.direction;

		m_editor_cam.aspect = static_cast<float>(m_render_extent.width) / static_cast<float>(m_render_extent.height);

		m_editor_cam.view = glm::lookAt(static_cast<glm::vec3>(m_editor_cam.position),
			static_cast<glm::vec3>(m_editor_cam.look_at), static_cast<glm::vec3>(m_editor_cam.up));

		m_editor_cam.proj = glm::perspective(maths::degToRad(m_editor_cam.fov_y), m_editor_cam.aspect,
			m_editor_cam.z_near, m_editor_cam.z_far);
		m_editor_cam.proj[1][1] *= -1;

		m_editor_cam.view_proj = m_editor_cam.proj * m_editor_cam.view;
	}

	void RenderSystemVK::update_camera_component(const TransformComponent3D& transform, CameraComponent& camera) const
	{
		// Calculate lookAt, right and up vectors
		camera.look_at = static_cast<glm::quat>(transform.orientation) * glm::vec3(0.0f, 0.0f, -1.0f);
		camera.right = static_cast<glm::quat>(transform.orientation) * glm::vec3(1.0f, 0.0f, 0.0f);
		camera.up = Vector3f(0.0f, 1.0f, 0.0f);

		camera.aspect = static_cast<float>(m_render_extent.width) / static_cast<float>(m_render_extent.height);

		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
			static_cast<glm::vec3>(camera.look_at), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(maths::degToRad(camera.fov_y), camera.aspect, camera.z_near, camera.z_far);
		camera.proj[1][1] *= -1;

		camera.view_proj = camera.proj * camera.view;
	}

	void RenderSystemVK::update_texture_descriptors()
	{
		if (m_initialized && current_frame_data().texture_descriptor_needs_updated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			build_texture_descriptor_info(m_tex_data, textureImageInfos);

			util::DescriptorBuilder::begin(m_global_render_data.descriptor_layout_cache,
			                               m_global_render_data.descriptor_allocator)
				.updateImages(5, textureImageInfos.size(), textureImageInfos.data(),
				              vk::DescriptorType::eCombinedImageSampler)
				.update(current_frame_data().global_descriptor);

			current_frame_data().texture_descriptor_needs_updated = false;
		}
	}

	void RenderSystemVK::prepare_scene_data()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = current_frame_data().camera_buffer;

		GPUCameraData camUBO = {};
		camUBO.proj = m_editor_cam.proj;
		camUBO.view = m_editor_cam.view;
		camUBO.viewProj = m_editor_cam.view_proj;

		memcpy(cameraBuffer.allocInfo.pMappedData, &camUBO, sizeof(GPUCameraData));

		// Prepare material data
		prepare_material_data();

		// Prepare object data
		prepare_object_data();

		// Prepare light data
		prepare_light_data();
	}

	void RenderSystemVK::prepare_material_data()
	{
		if (current_frame_data().copy_material_data_to_gpu)
		{
			std::vector<GPUMaterialInstanceData> materialData;
			materialData.reserve(m_material_registry.materialData().size());

			int idx = 0;
			for (auto& matData : m_material_registry.materialData())
			{
				// Update cached material data
				for (int i = 0; i < gNumTexturesPerMat; ++i)
				{
					if (matData.texIDs[i] != 0)
					{
						m_material_registry.getCachedMaterialData(matData.assetId).texIndices[i] = m_tex_data[matData.texIDs[i]].idx;
					}
				}

				materialData.push_back(m_material_registry.getCachedMaterialData(matData.assetId));

				matData.idx = idx;

				idx++;
			}

			util::copy_cpu_data_into_gpu_buffer(shared_from_this(), current_frame_data().material_buffer, 
			                                    materialData.size() * sizeof(GPUMaterialInstanceData), materialData.data());

			current_frame_data().copy_material_data_to_gpu = false;
		}
	}

	void RenderSystemVK::prepare_object_data()
	{
		if (!m_objects_to_refresh.empty())
		{
			const auto enkiTSSubSystem = mEngine->getSystem<core::EnkiTSSubsystem>();

			// Calculate t value for rendering interpolated position
			const double t = mEngine->accumulatedTime() / mEngine->timeStepFixed();

			std::vector<PuffinID> objectsToRefresh;
			objectsToRefresh.reserve(m_objects_to_refresh.size());

			for (const auto id : m_objects_to_refresh)
			{
				objectsToRefresh.push_back(id);
			}

			const auto numObjectsToRefresh = objectsToRefresh.size();

			const uint32_t numThreads = enkiTSSubSystem->getTaskScheduler()->GetNumTaskThreads();

			// Temp object vectors for writing to by threads
			std::vector<std::vector<std::pair<PuffinID, GPUObjectData>>> threadObjects;

			threadObjects.resize(numThreads);
			for (auto& threadObject : threadObjects)
			{
				threadObject.reserve(std::ceil(g_max_objects / numThreads));
			}

			const auto enttSubsystem = mEngine->getSystem<ecs::EnTTSubsystem>();
			const auto scene_graph = mEngine->getSystem<scene::SceneGraph>();
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

						tempTransform.position.x = transform.position.x;
						tempTransform.position.y = transform.position.y;
						tempTransform.position.z = 0.0;

						tempTransform.orientation = angleAxis(glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f));

						tempTransform.scale.x = transform.scale.x;
						tempTransform.scale.y = transform.scale.y;
						tempTransform.scale.z = 1.0f;
					}
					else
					{
						if (node)
						{
							tempTransform = node->global_transform_3d();
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
						physics::VelocityComponent3D& velocity = physics::VelocityComponent3D();

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
						Vector3f interpolatedPosition = tempTransform.position + velocity.linear * mEngine->timeStepFixed();
#endif

						position = maths::lerp(tempTransform.position, interpolatedPosition, t);
					}
					else
					{
						position = tempTransform.position;
					}

					GPUObjectData object;

					build_model_transform(position, tempTransform.orientation, tempTransform.scale, object.model);
					object.matIdx = m_material_registry.getMaterialData(mesh.mat_asset_id).idx;

					threadObjects[threadnum].emplace_back(entityID, object);
				}
			});

			task.m_MinRange = 500; // Try and ensure each thread gets a minimum of transforms matrices to calculate

			enkiTSSubSystem->getTaskScheduler()->AddTaskSetToPipe(&task);

			enkiTSSubSystem->getTaskScheduler()->WaitforTask(&task);

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

			util::copy_cpu_data_into_gpu_buffer(shared_from_this(), current_frame_data().object_buffer,
			                                    objects.size() * sizeof(GPUObjectData), objects.data());

			current_frame_data().copy_object_data_to_gpu = false;
		}
	}

	void RenderSystemVK::prepare_light_data()
	{
		// Prepare dynamic light data
		const auto registry = mEngine->getSystem<ecs::EnTTSubsystem>()->registry();

		const auto lightView = registry->view<const TransformComponent3D, const LightComponent>();

		std::vector<GPULightData> lights;

		int i = 0;

		for (auto [entity, transform, light] : lightView.each())
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= g_max_lights_vk)
			{
				break;
			}

			lights.emplace_back();

			lights[i].positionAndType.x = transform.position.x;
			lights[i].positionAndType.y = transform.position.y;
			lights[i].positionAndType.z = transform.position.z;
			lights[i].positionAndType.w = static_cast<int>(light.type);

			lights[i].direction.x = light.direction.x;
			lights[i].direction.y = light.direction.y;
			lights[i].direction.z = light.direction.z;
			lights[i].direction.w = 0.0f;

			lights[i].color.x = light.color.x;
			lights[i].color.y = light.color.y;
			lights[i].color.z = light.color.z;
			lights[i].color.w = 0.0f;

			lights[i].ambientSpecular.x = light.ambient_intensity;
			lights[i].ambientSpecular.y = light.specular_intensity;
			lights[i].ambientSpecular.z = light.specular_exponent;
			lights[i].ambientSpecular.w = 0.0f;

			lights[i].attenuation.x = light.constant_attenuation;
			lights[i].attenuation.y = light.linear_attenuation;
			lights[i].attenuation.z = light.quadratic_attenuation;
			lights[i].attenuation.w = 0.0f;

			lights[i].cutoffAngle.x = glm::cos(glm::radians(light.inner_cutoff_angle));
			lights[i].cutoffAngle.y = glm::cos(glm::radians(light.outer_cutoff_angle));
			lights[i].cutoffAngle.z = 0.0f;
			lights[i].cutoffAngle.w = 0.0f;

			i++;
		}

		// Copy light data to buffer
		util::copy_cpu_data_into_gpu_buffer(shared_from_this(), current_frame_data().light_buffer,
		                                    lights.size() * sizeof(GPULightData), lights.data());

		// Prepare light static data
		GPULightStaticData lightStaticUBO;
		lightStaticUBO.viewPosAndNumLights.x = m_editor_cam.position.x;
		lightStaticUBO.viewPosAndNumLights.y = m_editor_cam.position.y;
		lightStaticUBO.viewPosAndNumLights.z = m_editor_cam.position.z;
		lightStaticUBO.viewPosAndNumLights.w = i;

		// Copy light static data to buffer
		util::copy_cpu_data_into_gpu_buffer(shared_from_this(), current_frame_data().light_static_buffer,
		                                    sizeof(GPULightStaticData), &lightStaticUBO);
	}

	void RenderSystemVK::build_indirect_commands()
	{
		if (!m_renderables.empty())
		{
			std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
			indirectCmds.resize(g_max_objects);

			m_draw_batches.clear();
			m_draw_batches.reserve(m_material_registry.materialData().size());

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

			util::copy_cpu_data_into_gpu_buffer(shared_from_this(), current_frame_data().indirect_buffer,
			                                    indirectCmds.size() * sizeof(vk::DrawIndexedIndirectCommand), indirectCmds.data());
		}
	}

	vk::CommandBuffer& RenderSystemVK::record_shadow_command_buffer(uint32_t swapchain_idx)
	{
		auto& cmd = current_frame_data().shadow_command_buffer;

		cmd.reset();

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmd_begin_info = {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr
		};

		VK_CHECK(cmd.begin(&cmd_begin_info));

		

		cmd.end();

		return cmd;
	}

	void RenderSystemVK::draw_shadowmap(vk::CommandBuffer cmd, const AllocatedImage& depth_image, const vk::Extent2D& shadow_extent)
	{
		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange image_subresource_range = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreen_memory_barrier_to_depth = {
			vk::AccessFlagBits::eNone, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, {}, {},
			depth_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, 0, nullptr, 0, nullptr,
			1, & offscreen_memory_barrier_to_depth);

		vk::ClearValue depth_clear;
		depth_clear.depthStencil.depth = 1.f;

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			depth_image.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depth_clear
		};

		vk::RenderingInfoKHR render_info = {
			{}, vk::Rect2D{{0, 0}, shadow_extent}, 1, {}, 0, {}, &depth_attach_info
		};

		cmd.beginRendering(&render_info);

		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreen_memory_barrier_to_shader = {
			vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			depth_image.image, image_subresource_range
		};

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreen_memory_barrier_to_shader);
	}

	vk::CommandBuffer& RenderSystemVK::record_main_command_buffer(const uint32_t& swapchain_idx,
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
			color_image.imageView, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, color_clear
		};

		vk::RenderingAttachmentInfoKHR depth_attach_info = {
			depth_image.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
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

	void RenderSystemVK::draw_objects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		set_draw_parameters(cmd, renderExtent);

		bind_buffers_and_descriptors(cmd);

		// Make a indirect draw call for each material
		for (const auto& draw_batch : m_draw_batches)
		{
			draw_mesh_batch(cmd, draw_batch);
		}
	}

	void RenderSystemVK::set_draw_parameters(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		vk::Viewport viewport = {
			0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f
		};
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { {0, 0}, {renderExtent.width, renderExtent.height} };
		cmd.setScissor(0, 1, &scissor);
	}

	void RenderSystemVK::bind_buffers_and_descriptors(vk::CommandBuffer cmd)
	{
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_forward_pipeline_layout.get(), 0, 1,
			&current_frame_data().global_descriptor, 0, nullptr);

		GPUDrawPushConstant push_constant;
		push_constant.vertexBufferAddress = m_resource_manager->geometry_buffer()->vertex_buffer_address();

		cmd.pushConstants(m_forward_pipeline_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUDrawPushConstant), &push_constant);

		cmd.bindIndexBuffer(m_resource_manager->geometry_buffer()->index_buffer().buffer, 0, vk::IndexType::eUint32);
	}

	void RenderSystemVK::draw_mesh_batch(vk::CommandBuffer cmd, const MeshDrawBatch& meshDrawBatch)
	{
		// Use loaded material if id is valid, otherwise use default material
		if (meshDrawBatch.matID != gInvalidID)
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_material_registry.getMaterial(meshDrawBatch.matID).pipeline.get());
		}
		else
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_forward_pipeline.get());
		}

		vk::DeviceSize indirect_offset = meshDrawBatch.cmdIndex * sizeof(vk::DrawIndexedIndirectCommand);
		uint32_t draw_stride = sizeof(vk::DrawIndexedIndirectCommand);

		draw_indexed_indirect_command(cmd, current_frame_data().indirect_buffer.buffer, indirect_offset,
			meshDrawBatch.cmdCount, draw_stride);
	}

	void RenderSystemVK::draw_indexed_indirect_command(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer,
	                                                vk::DeviceSize offset,
	                                                uint32_t drawCount, uint32_t stride)
	{
		cmd.drawIndexedIndirect(indirectBuffer, offset, drawCount, stride);
		m_draw_calls++;
	}

	vk::CommandBuffer& RenderSystemVK::record_copy_command_buffer(uint32_t swapchainIdx)
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
			m_offscreen_data.allocImages[swapchainIdx].image, image_subresource_range
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

		cmd.blitImage(m_offscreen_data.allocImages[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
		              m_swapchain_data.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &image_blit_region,
		              vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreen_memory_barrier = {
			vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, {}, {},
			m_offscreen_data.allocImages[swapchainIdx].image, image_subresource_range
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

	vk::CommandBuffer& RenderSystemVK::record_imgui_command_buffer(uint32_t swapchainIdx,
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
			m_swapchain_data.imageViews[swapchainIdx], vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
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

	void RenderSystemVK::record_and_submit_commands(uint32_t swapchainIdx)
	{
		std::vector<vk::SubmitInfo> submits;

		vk::PipelineStageFlags wait_stage = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

		// Prepare shadow rendering command submit
		if (m_render_shadows)
		{
			auto& shadow_cmd = record_shadow_command_buffer(swapchainIdx);

			std::vector<vk::CommandBuffer*> shadow_commands = { &shadow_cmd };

			std::vector<vk::Semaphore*> shadow_wait_semaphores = { &current_frame_data().present_semaphore };
			std::vector<vk::Semaphore*> shadow_signal_semaphores = { &current_frame_data().shadow_semaphore };

			vk::SubmitInfo shadow_submit =
			{
				static_cast<uint32_t>(shadow_wait_semaphores.size()), *shadow_wait_semaphores.data(),
				&wait_stage, static_cast<uint32_t>(shadow_commands.size()), *shadow_commands.data(),
				static_cast<uint32_t>(shadow_signal_semaphores.size()), *shadow_signal_semaphores.data(), nullptr
			};

			submits.push_back(shadow_submit);
		}

		// Prepare main render command submit
		{
			auto& main_cmd = record_main_command_buffer(swapchainIdx, m_offscreen_data.extent,
			                                                        m_offscreen_data.allocImages[swapchainIdx],
			                                                        m_offscreen_data.allocDepthImage);

			std::vector<vk::CommandBuffer*> render_commands = { &main_cmd };

			std::vector<vk::Semaphore*> render_wait_semaphores = { &current_frame_data().present_semaphore };

			if (m_render_shadows)
			{
				render_wait_semaphores.push_back(&current_frame_data().shadow_semaphore);
			}

			std::vector<vk::Semaphore*> render_signal_semaphores = { &current_frame_data().render_semaphore };

			vk::SubmitInfo render_submit =
			{
				static_cast<uint32_t>(render_wait_semaphores.size()), *render_wait_semaphores.data(),
				&wait_stage, static_cast<uint32_t>(render_commands.size()), *render_commands.data(),
				static_cast<uint32_t>(render_signal_semaphores.size()), *render_signal_semaphores.data(), nullptr
			};

			submits.push_back(render_submit);
		}

		{
			if (mEngine->shouldRenderEditorUI())
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

		if (mEngine->shouldRenderEditorUI())
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

	void RenderSystemVK::build_model_transform(const Vector3f& position, const maths::Quat& orientation, const Vector3f& scale,
	                                         glm::mat4& model)
	{
		const auto scaleM = glm::scale(glm::mat4(1.0f), static_cast<glm::vec3>(scale));

		const auto orientM = glm::mat4_cast(static_cast<glm::quat>(orientation));

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

			meshData.vertexBuffer = util::initVertexBuffer(shared_from_this(), meshAsset->vertices().data(),
			                                               meshAsset->numVertices(), meshAsset->vertexSize());

			meshData.indexBuffer = util::initIndexBuffer(shared_from_this(), meshAsset->indices().data(),
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

	bool RenderSystemVK::load_texture(PuffinID texId, TextureDataVK& texData)
	{
		if (const auto texAsset = assets::AssetRegistry::get()->getAsset<assets::TextureAsset>(texId); texAsset && texAsset->load())
		{
			texData.assetId = texId;

			texData.sampler = m_global_render_data.texture_sampler;

			texData.texture = util::init_texture(shared_from_this(), texAsset->pixelData(),
			                                     texAsset->textureWidth(), texAsset->textureHeight(),
			                                     texAsset->textureSize(),
			                                     g_tex_format_vk.at(texAsset->textureFormat()));

			texAsset->unload();

			return true;
		}

		return false;
	}

	void RenderSystemVK::unload_texture(TextureDataVK& texData) const
	{
		m_device.destroyImageView(texData.texture.imageView);
		m_allocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void RenderSystemVK::build_texture_descriptor_info(PackedVector<PuffinID, TextureDataVK>& textureData,
	                                                   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.size());

		int idx = 0;
		for (auto& texData : textureData)
		{
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texData.texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}
}
