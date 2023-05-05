#include "Rendering/Vulkan/VKRenderSystem.hpp"

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_LOG

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <iostream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "MathHelpers.h"
#include "vk_mem_alloc.h"
#include "vk_mem_alloc.hpp"
#include "VkBootstrap.h"
#include "glm/glm.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "UI/Editor/UIManager.h"
#include "Window/WindowSubsystem.hpp"
#include "Engine/Engine.hpp"
#include "Rendering/Vulkan/VKHelpers.hpp"
#include "Components/Rendering/MeshComponent.h"
#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Components/TransformComponent.h"
#include "Components/Physics/VelocityComponent.hpp"
#include "Components/Rendering/LightComponent.h"
#include "Engine/SignalSubsystem.hpp"
#include "Input/InputSubsystem.h"
#include "Engine/EnkiTSSubsystem.hpp"
#include "Components/SceneObjectComponent.hpp"
#include "ECS/EnTTSubsystem.hpp"

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

namespace puffin::rendering::VK
{
	void VKRenderSystem::Init()
	{
		InitVulkan();

		InitSwapchain(m_swapchainData, m_oldSwapchainData.swapchain, m_windowSize);

		vk::Extent2D offscreenSize;
		if (m_engine->shouldRenderEditorUi())
		{
			const ImVec2 viewportSize = m_engine->uiManager()->GetWindowViewport()->GetViewportSize();
			offscreenSize.width = viewportSize.x;
			offscreenSize.height = viewportSize.y;
		}
		else
		{
			offscreenSize = m_windowSize;
		}

		InitOffscreen(m_offscreenData, offscreenSize, m_swapchainData.images.size());

		InitCommands();

		if (m_engine->shouldRenderEditorUi())
		{
			InitImGuiRenderPass();

			InitSwapchainFramebuffers(m_swapchainData);
		}

		InitSyncStructures();
		InitBuffers();
		InitSamplers();

		ProcessComponents();
		UpdateRenderData();

		InitDescriptors();
		InitPipelines();

		if (m_engine->shouldRenderEditorUi())
		{
			InitImGui();
			InitOffscreenImGuiTextures(m_offscreenData);
		}

		m_editorCam.position = { 0.0f, 0.0f, 15.0f };

		// Connect Signals
		const auto signalSubsystem = m_engine->getSubsystem<core::SignalSubsystem>();

		signalSubsystem->Connect<Input::InputEvent>(
			[&](const Input::InputEvent& inputEvent)
			{
				shared_from_this()->OnInputEvent(inputEvent);
			}
		);

		m_isInitialized = true;
	}

	void VKRenderSystem::Render()
	{
		ProcessEvents();

		ProcessComponents();

		UpdateEditorCamera();

		UpdateRenderData();

		Draw();

		// Clear all entity sets in mesh draw list
		for (auto& [fst, snd] : m_meshDrawList)
		{
			snd.clear();
		}
	}

	void VKRenderSystem::Cleanup()
	{
		m_device.waitIdle();

		if (m_isInitialized)
		{
			m_staticRenderData.combinedMeshBuffer.Cleanup();

			for (auto texData : m_texData)
			{
				UnloadTexture(texData);
			}

			m_texData.Clear();

			CleanSwapchain(m_swapchainData);

			if (m_oldSwapchainData.needsCleaned)
			{
				CleanSwapchain(m_oldSwapchainData);
			}

			CleanOffscreen(m_offscreenData);

			if (m_oldOffscreenData.needsCleaned)
			{
				CleanOffscreen(m_oldOffscreenData);
			}

			m_deletionQueue.Flush();

			m_isInitialized = false;
		}
	}

	void VKRenderSystem::OnInputEvent(const Input::InputEvent& inputEvent)
	{
		m_inputEvents.Push(inputEvent);
	}

	void VKRenderSystem::InitVulkan()
	{
		GLFWwindow* glfwWindow = m_engine->getSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetFramebufferSizeCallback(glfwWindow, FrameBufferResizeCallback);

		// Create Vulkan Instance
		int width, height;
		glfwGetWindowSize(glfwWindow, &width, &height);
		m_windowSize.width = static_cast<unsigned>(width);
		m_windowSize.height = static_cast<unsigned>(height);

		vkb::InstanceBuilder instBuilder;

		auto instRet = instBuilder.set_app_name("Puffin Engine")
			.request_validation_layers(enableValidationLayers)
			.require_api_version(1, 3, 0)
			.use_default_debug_messenger()
			.enable_extension("VK_KHR_get_physical_device_properties2")
			.build();

		vkb::Instance vkbInst = instRet.value();

		// Store Instance/Debug Messenger
		m_instance = vkbInst.instance;
		m_debugMessenger = vkbInst.debug_messenger;

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

		vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value();

		// Check for desired extension support
		std::vector<const char*> device_extensions =
		{
			"VK_EXT_memory_budget",
			"VK_KHR_dynamic_rendering",
		};

		// Select GPU
		vkb::PhysicalDeviceSelector selector { vkbInst };
		vkb::PhysicalDevice physDevice = selector
			.set_minimum_version(1, 3)
			.set_surface(m_surface)
			.set_required_features(physicalDeviceFeatures)
			.set_required_features_12(physicalDevice12Features)
			.add_required_extensions(device_extensions)
			.select()
			.value();

		// Create Vulkan Device
		vkb::DeviceBuilder deviceBuilder { physDevice };

		vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = { true };
		vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesKHR = { true };

		vkb::Device vkbDevice = deviceBuilder
			.add_pNext(&shaderDrawParametersFeatures)
			.add_pNext(&dynamicRenderingFeaturesKHR)
			.build()
			.value();

		m_device = vkbDevice.device;
		m_physicalDevice = physDevice.physical_device;

		// Get Graphics Queue
		m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// Init memory allocator
		vma::AllocatorCreateInfo allocatorInfo = { {}, m_physicalDevice, m_device};
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
					m_isReBAREnabled = true;
				}
			}
		}

		m_deletionQueue.PushFunction([=]()
		{
			m_allocator.destroy();

			m_device.destroy();
			m_instance.destroySurfaceKHR(m_surface);
			vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
			m_instance.destroy();
		});
	}

	void VKRenderSystem::InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain, const vk::Extent2D& swapchainExtent)
	{
		vkb::SwapchainBuilder swapchainBuilder { m_physicalDevice, m_device, m_surface};

		swapchainData.extent = swapchainExtent;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.use_default_format_selection()
			// Vsync present mode
			.set_old_swapchain(oldSwapchain)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(swapchainData.extent.width, swapchainData.extent.height)
			.set_image_usage_flags({ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT })
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

	void VKRenderSystem::InitOffscreen(OffscreenData& offscreenData, const vk::Extent2D& offscreenExtent, const int& offscreenImageCount)
	{
		offscreenData.extent = offscreenExtent;

		const vk::Extent3D imageExtent = { offscreenData.extent.width, offscreenData.extent.height, 1 };

		offscreenData.imageFormat = vk::Format::eR8G8B8A8Unorm;

		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, offscreenData.imageFormat, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
			{ vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | 
				vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst } };

		const vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo = { {}, {}, vk::ImageViewType::e2D, offscreenData.imageFormat, {}, subresourceRange };

		offscreenData.allocImages.resize(offscreenImageCount);
		for (int i = 0; i < offscreenImageCount; i++)
		{
			offscreenData.allocImages[i] = Util::CreateImage(shared_from_this(), imageInfo, imageViewInfo);
		}

		offscreenData.allocDepthImage = Util::InitDepthImage(shared_from_this(), imageExtent, vk::Format::eD32Sfloat);
	}

	void VKRenderSystem::InitSwapchainFramebuffers(SwapchainData& swapchainData)
	{
		vk::FramebufferCreateInfo fbInfo = { {}, m_renderPassImGui, 1, nullptr, swapchainData.extent.width, swapchainData.extent.height, 1 };

		// Grab number of images in swapchain
		const uint32_t swapchainImageCount = swapchainData.images.size();
		swapchainData.framebuffers.resize(swapchainImageCount);

		for (int i = 0; i < swapchainImageCount; i++)
		{
			std::array<vk::ImageView, 1> attachments = { swapchainData.imageViews[i] };

			fbInfo.pAttachments = attachments.data();
			fbInfo.attachmentCount = attachments.size();

			VK_CHECK(m_device.createFramebuffer(&fbInfo, nullptr, &swapchainData.framebuffers[i]));
		}
	}

	void VKRenderSystem::InitCommands()
	{
		vk::CommandPoolCreateInfo commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		vk::CommandBufferAllocateInfo commandBufferInfo = { {}, vk::CommandBufferLevel::ePrimary, 1 };

		// Init Main Command Pools/Buffers
		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_frameRenderData[i].commandPool));

			commandBufferInfo.commandPool = m_frameRenderData[i].commandPool;
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frameRenderData[i].mainCommandBuffer));
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frameRenderData[i].copyCommandBuffer));
			VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_frameRenderData[i].imguiCommandBuffer));

			m_deletionQueue.PushFunction([=]()
			{
				m_device.destroyCommandPool(m_frameRenderData[i].commandPool);
			});
		}

		// Init Upload Context Command Pool/Buffer
		commandPoolInfo = { vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily };
		VK_CHECK(m_device.createCommandPool(&commandPoolInfo, nullptr, &m_uploadContext.commandPool));

		commandBufferInfo = { m_uploadContext.commandPool, vk::CommandBufferLevel::ePrimary, 1 };
		VK_CHECK(m_device.allocateCommandBuffers(&commandBufferInfo, &m_uploadContext.commandBuffer));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyCommandPool(m_uploadContext.commandPool);
		});
	}

	void VKRenderSystem::InitImGuiRenderPass()
	{
		// Setup Attachments

		vk::AttachmentDescription colorAttachment =
		{
			{}, m_swapchainData.imageFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference colorAttachmentRef = { 0, vk::ImageLayout::eColorAttachmentOptimal };

		vk::SubpassDescription subpass = { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
			1, &colorAttachmentRef, {}, {} };

		std::array<vk::AttachmentDescription, 1> attachments = { colorAttachment };

		// Setup Dependencies
		vk::SubpassDependency colorDependency = { VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, vk::AccessFlagBits::eColorAttachmentWrite };

		std::array<vk::SubpassDependency, 1> dependencies = { colorDependency };

		vk::RenderPassCreateInfo renderPassInfo = { {}, attachments.size(), attachments.data(), 1, &subpass,
			dependencies.size(), dependencies.data() };

		// Create Render Pass

		VK_CHECK(m_device.createRenderPass(&renderPassInfo, nullptr, &m_renderPassImGui));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyRenderPass(m_renderPassImGui);
		});
	}

	void VKRenderSystem::InitSyncStructures()
	{
		vk::FenceCreateInfo fenceCreateInfo = { vk::FenceCreateFlagBits::eSignaled, nullptr };
		vk::SemaphoreCreateInfo semaphoreCreateInfo = { {}, nullptr };

		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_frameRenderData[i].renderFence));

			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].renderSemaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].copySemaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].imguiSemaphore));
			VK_CHECK(m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_frameRenderData[i].presentSemaphore));

			m_deletionQueue.PushFunction([=]()
			{
				m_device.destroyFence(m_frameRenderData[i].renderFence);

				m_device.destroySemaphore(m_frameRenderData[i].renderSemaphore);
				m_device.destroySemaphore(m_frameRenderData[i].copySemaphore);
				m_device.destroySemaphore(m_frameRenderData[i].imguiSemaphore);
				m_device.destroySemaphore(m_frameRenderData[i].presentSemaphore);
			});
		}

		// Init Upload Context Fence
		fenceCreateInfo = vk::FenceCreateInfo{ {}, nullptr };
		VK_CHECK(m_device.createFence(&fenceCreateInfo, nullptr, &m_uploadContext.uploadFence));

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyFence(m_uploadContext.uploadFence);
		});
	}

	void VKRenderSystem::InitBuffers()
	{
		m_staticRenderData.combinedMeshBuffer.Init(shared_from_this(), sizeof(VertexPNTV32));

		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			// Indirect Buffer
			m_frameRenderData[i].indirectBuffer = Util::CreateBuffer(m_allocator, sizeof(vk::DrawIndexedIndirectCommand) * G_MAX_OBJECTS,
				vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				vma::MemoryUsage::eAuto, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

			// Global Buffers
			m_frameRenderData[i].cameraBuffer = Util::CreateBuffer(m_allocator, sizeof(GPUCameraData),
				vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eAuto, 
				{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			m_frameRenderData[i].lightBuffer = Util::CreateBuffer(m_allocator, sizeof(GPULightData) * G_MAX_LIGHTS,
				vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAuto, 
				{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			m_frameRenderData[i].lightStaticBuffer = Util::CreateBuffer(m_allocator, sizeof(GPULightStaticData),
				vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eAuto, 
				{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			m_frameRenderData[i].objectBuffer = Util::CreateBuffer(m_allocator, sizeof(GPUObjectData) * G_MAX_OBJECTS,
				vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAuto,
				{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			// Material Buffers

			// Object Buffers

			m_deletionQueue.PushFunction([=]()
			{
				m_allocator.destroyBuffer(m_frameRenderData[i].objectBuffer.buffer, m_frameRenderData[i].objectBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].lightStaticBuffer.buffer, m_frameRenderData[i].lightStaticBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].lightBuffer.buffer, m_frameRenderData[i].lightBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].cameraBuffer.buffer, m_frameRenderData[i].cameraBuffer.allocation);
				m_allocator.destroyBuffer(m_frameRenderData[i].indirectBuffer.buffer, m_frameRenderData[i].indirectBuffer.allocation);
			});
		}
	}

	void VKRenderSystem::InitSamplers()
	{
		const vk::SamplerCreateInfo samplerInfo = {};

		m_staticRenderData.textureSampler = m_device.createSampler(samplerInfo);

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroySampler(m_staticRenderData.textureSampler, nullptr);
		});
	}

	void VKRenderSystem::InitDescriptors()
	{
		// Descriptor Allocator/Cache

		m_staticRenderData.descriptorAllocator = std::make_shared<Util::DescriptorAllocator>(m_device);
		m_staticRenderData.descriptorLayoutCache = std::make_shared<Util::DescriptorLayoutCache>(m_device);

		for (int i = 0; i < G_BUFFERED_FRAMES; i++)
		{
			// Global Descriptors

			vk::DescriptorBufferInfo cameraBufferInfo = { m_frameRenderData[i].cameraBuffer.buffer, 0, sizeof(GPUCameraData) };
			vk::DescriptorBufferInfo objectBufferInfo = { m_frameRenderData[i].objectBuffer.buffer, 0, sizeof(GPUObjectData) * G_MAX_OBJECTS };
			vk::DescriptorBufferInfo lightBufferInfo = { m_frameRenderData[i].lightBuffer.buffer, 0, sizeof(GPULightData) * G_MAX_LIGHTS };
			vk::DescriptorBufferInfo lightStaticBufferInfo = { m_frameRenderData[i].lightStaticBuffer.buffer, 0, sizeof(GPULightStaticData) };

			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(m_texData, textureImageInfos);

			Util::DescriptorBuilder::Begin(m_staticRenderData.descriptorLayoutCache, m_staticRenderData.descriptorAllocator)
				.BindBuffer(0, &cameraBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
				.BindBuffer(1, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.BindBuffer(2, &lightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(3, &lightStaticBufferInfo, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindImages(4, textureImageInfos.size(), textureImageInfos.data(), 
					vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
				.Build(m_frameRenderData[i].globalDescriptor, m_staticRenderData.globalSetLayout);

			// Material Descriptors

		}

		m_deletionQueue.PushFunction([=]()
		{
			m_staticRenderData.descriptorLayoutCache = nullptr;
			m_staticRenderData.descriptorAllocator = nullptr;
		});
	}

	void VKRenderSystem::InitPipelines()
	{
		BuildForwardRendererPipeline();
	}

	void VKRenderSystem::BuildForwardRendererPipeline()
	{
		m_forwardVertMod = Util::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_vs.spv" };
		m_forwardFragMod = Util::ShaderModule{ m_device, "C:\\Projects\\PuffinEngine\\bin\\vulkan\\forward_shading\\forward_shading_fs.spv" };

		Util::PipelineLayoutBuilder plb{};
		m_forwardPipelineLayout = plb
			.DescriptorSetLayout(m_staticRenderData.globalSetLayout)
			.CreateUnique(m_device);

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = { {}, true, true,
			vk::CompareOp::eLessOrEqual, false, false, {}, {}, 0.0f, 1.0f };

		vk::PipelineRenderingCreateInfoKHR pipelineRenderInfo = { 0, m_offscreenData.imageFormat, m_offscreenData.allocDepthImage.format };

		Util::PipelineBuilder pb{ m_windowSize.width, m_windowSize.height };
		m_forwardPipeline = pb
			// Define dynamic state which can change each frame (currently viewport and scissor size)
			.DynamicState(vk::DynamicState::eViewport)
			.DynamicState(vk::DynamicState::eScissor)
			// Define vertex/fragment shaders
			.Shader(vk::ShaderStageFlagBits::eVertex, m_forwardVertMod)
			.Shader(vk::ShaderStageFlagBits::eFragment, m_forwardFragMod)
			.DepthStencilState(depthStencilInfo)
			// Define vertex binding/attributes
			.VertexLayout(VertexPNTV32::GetLayoutVK())
			// Add rendering info struct
			.AddPNext(&pipelineRenderInfo)
			// Create pipeline
			.CreateUnique(m_device, m_pipelineCache, *m_forwardPipelineLayout, nullptr);

		m_device.destroyShaderModule(m_forwardVertMod.Module());
		m_device.destroyShaderModule(m_forwardFragMod.Module());

		m_deletionQueue.PushFunction([=]()
		{
			m_forwardPipelineLayout = {};
			m_forwardPipeline = {};
		});
	}

	void VKRenderSystem::InitImGui()
	{
		// Create Descriptor Pool for ImGui
		vk::DescriptorPoolSize poolSizes[] =
		{
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};

		vk::DescriptorPoolCreateInfo poolInfo = 
		{
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, std::size(poolSizes), poolSizes
		};

		vk::DescriptorPool imguiPool;
		VK_CHECK(m_device.createDescriptorPool(&poolInfo, nullptr, &imguiPool));

		// Initialize imgui for GLFW
		GLFWwindow* glfwWindow = m_engine->getSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		// Initialize imgui for Vulkan
		ImGui_ImplVulkan_InitInfo initInfo = { m_instance, m_physicalDevice, m_device, m_graphicsQueueFamily,
			m_graphicsQueue, m_pipelineCache, imguiPool,
			0, 3, 3, VK_SAMPLE_COUNT_1_BIT, nullptr };

		ImGui_ImplVulkan_Init(&initInfo, m_renderPassImGui);

		// Upload ImGui font textures
		Util::ImmediateSubmit(shared_from_this(), [=](vk::CommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

		// Clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		m_deletionQueue.PushFunction([=]()
		{
			m_device.destroyDescriptorPool(imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplGlfw_Shutdown();
		});
	}

	void VKRenderSystem::InitOffscreenImGuiTextures(OffscreenData& offscreenData)
	{
		offscreenData.viewportTextures.resize(offscreenData.allocImages.size());

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			offscreenData.viewportTextures[i] = static_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(m_staticRenderData.textureSampler,
				offscreenData.allocImages[i].imageView, static_cast<VkImageLayout>(vk::ImageLayout::eShaderReadOnlyOptimal)));
		}
	}

	void VKRenderSystem::ProcessEvents()
	{
		Input::InputEvent inputEvent;
		while (m_inputEvents.Pop(inputEvent))
		{
			if (inputEvent.actionName == "CamMoveLeft")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveLeft = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveLeft = false;
				}
			}

			if (inputEvent.actionName == "CamMoveRight")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveRight = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveRight = false;
				}
			}

			if (inputEvent.actionName == "CamMoveForward")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveForward = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveForward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveBackward")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveBackward = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveBackward = false;
				}
			}

			if (inputEvent.actionName == "CamMoveUp")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveUp = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveUp = false;
				}
			}

			if (inputEvent.actionName == "CamMoveDown")
			{
				if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
				{
					m_moveDown = true;
				}
				else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
				{
					m_moveDown = false;
				}
			}
		}
	}

	void VKRenderSystem::ProcessComponents()
	{
		auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		auto meshView = registry->view<const SceneObjectComponent, const TransformComponent, const MeshComponent>();

		for (auto [entity, object, transform, mesh] : meshView.each())
		{
			m_meshDrawList[mesh.meshAssetId].insert(object.uuid);
			m_texDrawList[mesh.textureAssetId].insert(object.uuid);
		}

		auto cameraView = registry->view<const SceneObjectComponent, const TransformComponent, CameraComponent>();

		for (auto [entity, object, transform, camera] : cameraView.each())
		{
			UpdateCameraComponent(transform, camera);
		}

		/*auto lightView = registry->view<const ECS::SceneObjectComponent, const TransformComponent, LightComponent>();
		
		for (auto [entity, object, transform, light] : lightView.each())
		{
			
		}*/
	}

	void VKRenderSystem::UpdateEditorCamera()
	{
		const auto inputSubsystem = m_engine->getSubsystem<Input::InputSubsystem>();

		if (inputSubsystem->IsCursorLocked())
		{
			// Camera Movement
			if (m_moveLeft && !m_moveRight)
			{
				m_editorCam.position += m_editorCam.right * m_editorCam.speed * m_engine->deltaTime();
			}

			if (m_moveRight && !m_moveLeft)
			{
				m_editorCam.position -= m_editorCam.right * m_editorCam.speed * m_engine->deltaTime();
			}

			if (m_moveForward && !m_moveBackward)
			{
				m_editorCam.position += m_editorCam.direction * m_editorCam.speed * m_engine->deltaTime();
			}

			if (m_moveBackward && !m_moveForward)
			{
				m_editorCam.position -= m_editorCam.direction * m_editorCam.speed * m_engine->deltaTime();
			}

			if (m_moveUp && !m_moveDown)
			{
				m_editorCam.position += m_editorCam.up * m_editorCam.speed * m_engine->deltaTime();
			}

			if (m_moveDown && !m_moveUp)
			{
				m_editorCam.position -= m_editorCam.up * m_editorCam.speed * m_engine->deltaTime();
			}

			// Mouse Rotation
			m_editorCam.yaw += inputSubsystem->GetMouseXOffset();
			m_editorCam.pitch -= inputSubsystem->GetMouseYOffset();

			if (m_editorCam.pitch > 89.0f)
				m_editorCam.pitch = 89.0f;

			if (m_editorCam.pitch < -89.0f)
				m_editorCam.pitch = -89.0f;

			// Calculate Direction vector from yaw and pitch of camera
			m_editorCam.direction.x = cos(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));
			m_editorCam.direction.y = sin(Maths::DegreesToRadians(m_editorCam.pitch));
			m_editorCam.direction.z = sin(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));

			m_editorCam.direction.Normalise();
		}

		// Calculate Right, Up and LookAt vectors
		m_editorCam.right = m_editorCam.up.Cross(m_editorCam.direction).Normalised();
		m_editorCam.lookAt = m_editorCam.position + m_editorCam.direction;

		m_editorCam.aspect = static_cast<float>(m_windowSize.width) / static_cast<float>(m_windowSize.height);

		m_editorCam.view = glm::lookAt(static_cast<glm::vec3>(m_editorCam.position),
			static_cast<glm::vec3>(m_editorCam.lookAt), static_cast<glm::vec3>(m_editorCam.up));

		m_editorCam.proj = glm::perspective(Maths::DegreesToRadians(m_editorCam.fovY), m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar);
		m_editorCam.proj[1][1] *= -1;

		m_editorCam.viewProj = m_editorCam.proj * m_editorCam.view;
	}

	void VKRenderSystem::UpdateRenderData()
	{
		std::set<UUID> meshesToBeRemoved;

		for (const auto& [fst, snd] : m_meshDrawList)
		{
			const auto staticMesh = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(fst));
			m_staticRenderData.combinedMeshBuffer.AddMesh(staticMesh);

			// Check if mesh is still in use by any in-flight frames
			if (!snd.empty())
			{
				GetCurrentFrameData().renderedMeshes.insert(fst);
			}
			else
			{
				GetCurrentFrameData().renderedMeshes.erase(fst);

				bool meshStillBeingRendered = false;
				for (const auto& frameRenderData : m_frameRenderData)
				{
					if (frameRenderData.renderedMeshes.count(fst) == 1)
						meshStillBeingRendered = true;
				}

				if (!meshStillBeingRendered)
				{
					meshesToBeRemoved.insert(fst);
				}
			}
		}

		// Remove all marked meshes from combined buffer
		if (!meshesToBeRemoved.empty())
		{
			// Currently disabled as RemoveMeshes call caused scene rendering to be messed up, will need investigated
			//m_staticRenderData.combinedMeshBuffer.RemoveMeshes(meshesToBeRemoved);

			// Remove marked meshes from draw list
			for (const auto& meshID : meshesToBeRemoved)
			{
				m_meshDrawList.erase(meshID);
			}

			meshesToBeRemoved.clear();
		}

		bool textureDescriptorNeedsUpdated = false;
		for (const auto& [fst, snd] : m_texDrawList)
		{
			if (!m_texData.Contains(fst))
			{
				TextureData texData;
				LoadTexture(fst, texData);

				m_texData.Insert(fst, texData);

				textureDescriptorNeedsUpdated = true;
			}
		}

		if (textureDescriptorNeedsUpdated)
		{
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				m_frameRenderData[i].textureDescriptorNeedsupdated = true;
			}
		}
	}

	void VKRenderSystem::Draw()
	{
		// Wait until GPU has finished rendering last frame. Timeout of 1 second
		VK_CHECK(m_device.waitForFences(1, &GetCurrentFrameData().renderFence, true, 1000000000));
		VK_CHECK(m_device.resetFences(1, &GetCurrentFrameData().renderFence));

		if (m_engine->shouldRenderEditorUi())
		{
			const ImVec2 viewportSize = m_engine->uiManager()->GetWindowViewport()->GetViewportSize();
			if (viewportSize.x != m_offscreenData.extent.width ||
				viewportSize.y != m_offscreenData.extent.height)
			{
				m_offscreenData.resized = true;
			}
		}

		RecreateSwapchain();
		RecreateOffscreen();

		m_drawCalls = 0;

		uint32_t swapchainImageIdx;
		VK_CHECK(m_device.acquireNextImageKHR(m_swapchainData.swapchain, 1000000000, GetCurrentFrameData().presentSemaphore, nullptr, &swapchainImageIdx));

		// Prepare textures, scene data & indirect commands for rendering
		UpdateTextureDescriptors();
		PrepareSceneData();
		BuildIndirectCommands();

		if (m_engine->shouldRenderEditorUi())
		{
			//m_engine->GetUIManager()->DrawUI(m_engine->GetDeltaTime());

			m_engine->uiManager()->GetWindowViewport()->Draw(m_offscreenData.viewportTextures[swapchainImageIdx]);

			ImGui::Render();
		}

		RecordAndSubmitCommands(swapchainImageIdx);

		m_frameNumber++;
	}

	void VKRenderSystem::UpdateCameraComponent(const TransformComponent& transform, CameraComponent& camera)
	{
		// Calculate Right, Up and LookAt vectors
		camera.right = camera.up.Cross(transform.rotation.GetXYZ()).Normalised();
		camera.lookAt = transform.position + transform.rotation.GetXYZ();

		camera.aspect = (float)m_windowSize.width / (float)m_windowSize.height;

		camera.view = glm::lookAt(static_cast<glm::vec3>(transform.position),
			static_cast<glm::vec3>(camera.lookAt), static_cast<glm::vec3>(camera.up));

		camera.proj = glm::perspective(Maths::DegreesToRadians(camera.fovY), camera.aspect, camera.zNear, camera.zFar);
		camera.proj[1][1] *= -1;

		camera.viewProj = camera.proj * camera.view;
	}

	void VKRenderSystem::RecreateSwapchain()
	{
		// Recreate swapchain when window is resized
		if (m_swapchainData.resized == true)
		{
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				m_frameRenderData[i].swapchainNeedsUpdated = true;
			}

			m_oldSwapchainData = m_swapchainData;
			m_oldSwapchainData.needsCleaned = true;

			InitSwapchain(m_swapchainData, m_oldSwapchainData.swapchain, m_windowSize);

			if (m_engine->shouldRenderEditorUi())
			{
				InitSwapchainFramebuffers(m_swapchainData);
			}

			m_swapchainData.resized = false;
		}

		if (m_oldSwapchainData.needsCleaned == true)
		{
			if (GetCurrentFrameData().swapchainNeedsUpdated == true)
			{
				GetCurrentFrameData().swapchainNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				if (m_frameRenderData[i].swapchainNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == G_BUFFERED_FRAMES)
			{
				CleanSwapchain(m_oldSwapchainData);

				m_oldSwapchainData.needsCleaned = false;
			}
		}
	}

	void VKRenderSystem::CleanSwapchain(SwapchainData& swapchainData)
	{
		for (int i = 0; i < swapchainData.imageViews.size(); i++)
		{
			if (m_engine->shouldRenderEditorUi())
			{
				m_device.destroyFramebuffer(swapchainData.framebuffers[i]);
			}

			m_device.destroyImageView(swapchainData.imageViews[i]);
		}

		m_device.destroySwapchainKHR(swapchainData.swapchain);
	}

	void VKRenderSystem::RecreateOffscreen()
	{
		if (m_offscreenData.resized == true)
		{
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				m_frameRenderData[i].offscreenNeedsUpdated = true;
			}

			m_oldOffscreenData = m_offscreenData;
			m_oldOffscreenData.needsCleaned = true;

			vk::Extent2D offscreenSize;
			if (m_engine->shouldRenderEditorUi())
			{
				const ImVec2 viewportSize = m_engine->uiManager()->GetWindowViewport()->GetViewportSize();
				offscreenSize.width = static_cast<uint32_t>(viewportSize.x);
				offscreenSize.height = static_cast<uint32_t>(viewportSize.y);
			}
			else
			{
				offscreenSize = m_windowSize;
			}

			InitOffscreen(m_offscreenData, offscreenSize, m_swapchainData.images.size());

			if (m_engine->shouldRenderEditorUi())
			{
				InitOffscreenImGuiTextures(m_offscreenData);
			}

			m_offscreenData.resized = false;
		}

		if (m_oldOffscreenData.needsCleaned == true)
		{
			if (GetCurrentFrameData().offscreenNeedsUpdated == true)
			{
				GetCurrentFrameData().offscreenNeedsUpdated = false;
			}

			int framesUsingNewSwapchain = 0;
			for (int i = 0; i < G_BUFFERED_FRAMES; i++)
			{
				if (m_frameRenderData[i].offscreenNeedsUpdated == false)
				{
					framesUsingNewSwapchain++;
				}
			}

			if (framesUsingNewSwapchain == G_BUFFERED_FRAMES)
			{
				CleanOffscreen(m_oldOffscreenData);

				m_oldOffscreenData.needsCleaned = false;
			}
		}
	}

	void VKRenderSystem::CleanOffscreen(OffscreenData& offscreenData)
	{
		m_device.destroyImageView(offscreenData.allocDepthImage.imageView);
		m_allocator.destroyImage(offscreenData.allocDepthImage.image, offscreenData.allocDepthImage.allocation);

		for (int i = 0; i < offscreenData.allocImages.size(); i++)
		{
			if (m_engine->shouldRenderEditorUi())
			{
				ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(offscreenData.viewportTextures[i]));
			}

			m_device.destroyImageView(offscreenData.allocImages[i].imageView);
			m_allocator.destroyImage(offscreenData.allocImages[i].image, offscreenData.allocImages[i].allocation);
		}
	}

	void VKRenderSystem::UpdateTextureDescriptors()
	{
		if (m_isInitialized && GetCurrentFrameData().textureDescriptorNeedsupdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(m_texData, textureImageInfos);

			Util::DescriptorBuilder::Begin(m_staticRenderData.descriptorLayoutCache, m_staticRenderData.descriptorAllocator)
				.UpdateImages(4, textureImageInfos.size(), textureImageInfos.data(),
					vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().globalDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsupdated = false;
		}
	}

	void VKRenderSystem::PrepareSceneData()
	{
		// Prepare camera data
		const AllocatedBuffer& cameraBuffer = GetCurrentFrameData().cameraBuffer;

		GPUCameraData camUBO = {};
		camUBO.proj = m_editorCam.proj;
		camUBO.view = m_editorCam.view;
		camUBO.viewProj = m_editorCam.viewProj;

		memcpy(cameraBuffer.allocInfo.pMappedData, &camUBO, sizeof(GPUCameraData));

		// Prepare object data
		PrepareObjectData();

		// Prepare light data
		PrepareLightData();
	}

	void VKRenderSystem::PrepareObjectData()
	{
		const auto enkiTSSubSystem = m_engine->getSubsystem<core::EnkiTSSubsystem>();

		const AllocatedBuffer& objectBuffer = GetCurrentFrameData().objectBuffer;

		std::vector<GPUObjectData> objects = {};
		objects.reserve(G_MAX_OBJECTS);

		// Calculate t value for rendering interpolated position
		const double t = m_engine->accumulatedTime() / m_engine->timeStepFixed();

		std::vector<UUID> entities;
		entities.reserve(G_MAX_OBJECTS);

		int numObjects = 0;

		for (const auto& [fst, snd] : m_meshDrawList)
		{
			for (const auto entityID : snd)
			{
				entities.emplace_back(entityID);

				numObjects++;
			}
		}

		objects.resize(numObjects);

		const uint32_t numThreads = enkiTSSubSystem->GetTaskScheduler()->GetNumTaskThreads();

		std::vector<std::vector<std::pair<GPUObjectData, uint32_t>>> threadObjects; // Temp object vectors for writing to by threads

		threadObjects.resize(numThreads);
		for (int idx = 0; idx < threadObjects.size(); idx++)
		{
			threadObjects[idx].reserve(500);
		}

		auto enttSubsystem = m_engine->getSubsystem<ECS::EnTTSubsystem>();
		auto registry = enttSubsystem->Registry();

		enki::TaskSet task(numObjects, [&](enki::TaskSetPartition range, uint32_t threadnum)
		{
			uint32_t numObjectsForThread = range.end - range.start;

			for (uint32_t objectIdx = range.start; objectIdx < range.end; objectIdx++)
			{
				auto entity = enttSubsystem->GetEntity(entities[objectIdx]);

				const auto& transform = registry->get<TransformComponent>(entity);
				const auto& mesh = registry->get<MeshComponent>(entity);

#ifdef PFN_USE_DOUBLE_PRECISION
				Vector3d position = { 0.0 };
#else
				Vector3f position = { 0.0f };
#endif

				if (registry->all_of<physics::VelocityComponent>(entity))
				{
					const auto& velocity = registry->get<physics::VelocityComponent>(entity);

#ifdef PFN_USE_DOUBLE_PRECISION
					Vector3d interpolatedPosition = transform.position + velocity.linear * m_engine->timeStepFixed();
#else
					Vector3f interpolatedPosition = transform.position + velocity.linear * m_engine->GetTimeStep();
#endif

					position = Maths::Lerp(transform.position, interpolatedPosition, t);
				}
				else
				{
					position = transform.position;
				}

				GPUObjectData object;

				BuildModelTransform(position, transform.rotation.EulerAnglesRad(), transform.scale, object.model);
				object.texIndex = m_texData[mesh.textureAssetId].idx;

				threadObjects[threadnum].emplace_back(object, objectIdx);
			}
		});

		task.m_MinRange = 500; // Try and ensure each thread gets

		enkiTSSubSystem->GetTaskScheduler()->AddTaskSetToPipe(&task);

		enkiTSSubSystem->GetTaskScheduler()->WaitforTask(&task);

		for (const auto& tempThreadObjects : threadObjects)
		{
			for (const auto& [fst, snd] : tempThreadObjects)
			{
				objects[snd] = fst;
			}
		}

		const auto* objectData = objects.data();

		std::copy_n(objectData, numObjects, static_cast<GPUObjectData*>(objectBuffer.allocInfo.pMappedData));
	}

	void VKRenderSystem::PrepareLightData()
	{
		const AllocatedBuffer& lightBuffer = GetCurrentFrameData().lightBuffer;

		auto* lightSSBO = static_cast<GPULightData*>(lightBuffer.allocInfo.pMappedData);

		int i = 0;

		auto registry = m_engine->getSubsystem<ECS::EnTTSubsystem>()->Registry();

		auto lightView = registry->view<const SceneObjectComponent, const TransformComponent, const LightComponent>();

		for (auto [entity, object, transform, light] : lightView.each()) 
		{
			// Break out of loop of maximum number of lights has been reached
			if (i >= G_MAX_LIGHTS)
			{
				break;
			}

			lightSSBO[i].position = static_cast<glm::vec3>(transform.position);
			lightSSBO[i].direction = static_cast<glm::vec3>(transform.rotation.GetXYZ());
			lightSSBO[i].color = static_cast<glm::vec3>(light.color);
			lightSSBO[i].ambientSpecular = glm::vec3(light.ambientIntensity, light.specularIntensity, light.specularExponent);
			lightSSBO[i].attenuation = glm::vec3(light.constantAttenuation, light.linearAttenuation, light.quadraticAttenuation);
			lightSSBO[i].cutoffAngle = glm::vec3(
				glm::cos(glm::radians(light.innerCutoffAngle)),
				glm::cos(glm::radians(light.outerCutoffAngle)), 0.0f);
			lightSSBO[i].type = static_cast<int>(light.type);

			i++;
		}

		// Prepare light static data
		const AllocatedBuffer& lightStaticBuffer = GetCurrentFrameData().lightStaticBuffer;

		GPULightStaticData lightStaticUBO = {};
		lightStaticUBO.numLights = i;
		lightStaticUBO.viewPos = static_cast<glm::vec3>(m_editorCam.position);

		memcpy(lightStaticBuffer.allocInfo.pMappedData, &lightStaticUBO, sizeof(GPULightStaticData));
	}

	void VKRenderSystem::BuildIndirectCommands()
	{
		AllocatedBuffer& indirectBuffer = GetCurrentFrameData().indirectBuffer;

		std::vector<vk::DrawIndexedIndirectCommand> indirectCmds = {};
		indirectCmds.resize(G_MAX_OBJECTS);

		int idx = 0;

		auto enttSubsystem = m_engine->getSubsystem<ECS::EnTTSubsystem>();
		auto registry = enttSubsystem->Registry();

		for (const auto [fst, snd] : m_meshDrawList)
		{
			for (const auto entityID : snd)
			{
				auto entity = enttSubsystem->GetEntity(entityID);

				const auto& mesh = registry->get<MeshComponent>(entity);

				indirectCmds[idx].vertexOffset = m_staticRenderData.combinedMeshBuffer.MeshVertexOffset(mesh.meshAssetId);
				indirectCmds[idx].firstIndex = m_staticRenderData.combinedMeshBuffer.MeshIndexOffset(mesh.meshAssetId);
				indirectCmds[idx].indexCount = m_staticRenderData.combinedMeshBuffer.MeshIndexCount(mesh.meshAssetId);
				indirectCmds[idx].firstInstance = idx;
				indirectCmds[idx].instanceCount = 1;

				idx++;
			}
		}

		GetCurrentFrameData().drawCount = idx;

		const auto* indirectData = indirectCmds.data();

		std::copy_n(indirectData, idx, static_cast<vk::DrawIndexedIndirectCommand*>(indirectBuffer.allocInfo.pMappedData));
	}

	vk::CommandBuffer VKRenderSystem::RecordMainCommandBuffer(const uint32_t& swapchainIdx, const vk::Extent2D& renderExtent, const AllocatedImage&
	                                                          colorImage, const AllocatedImage& depthImage)
	{
		vk::CommandBuffer cmd = GetCurrentFrameData().mainCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr };

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		// Transition color image to color attachment optimal
		vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		vk::ImageMemoryBarrier offscreenMemoryBarrierToColor = { vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,{}, {},
				colorImage.image, imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, 0, nullptr, 0, nullptr,
			1, & offscreenMemoryBarrierToColor);

		vk::ClearValue clearValue;
		clearValue.color = { 0.0f, 0.7f, 0.9f, 1.0f };

		vk::ClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;

		std::array<vk::ClearValue, 2> clearValues = { clearValue, depthClear };

		// Begin Rendering
		vk::RenderingAttachmentInfoKHR colorAttachInfo = { colorImage.imageView, vk::ImageLayout::eColorAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clearValue };

		vk::RenderingAttachmentInfoKHR depthAttachInfo = { depthImage.imageView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ResolveModeFlagBits::eNone, {},
			vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, depthClear };

		vk::RenderingInfoKHR renderInfo = { {}, vk::Rect2D{ {0, 0}, renderExtent }, 1, {}, 1, &colorAttachInfo, &depthAttachInfo };

		cmd.beginRendering(&renderInfo);

		DrawObjects(cmd, renderExtent);

		// End Rendering
		cmd.endRendering();

		// Transition layout to Shader Read Optimal
		vk::ImageMemoryBarrier offscreenMemoryBarrierToShader = { vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,{}, {},
				colorImage.image, imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, 0, nullptr, 0, nullptr,
			1, & offscreenMemoryBarrierToShader);

		// Finish command buffer recording
		cmd.end();

		return cmd;
	}

	void VKRenderSystem::DrawObjects(vk::CommandBuffer cmd, const vk::Extent2D& renderExtent)
	{
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_forwardPipeline.get());

		vk::Viewport viewport = { 0, 0, static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height), 0.1f, 1.0f };
		cmd.setViewport(0, 1, &viewport);

		vk::Rect2D scissor = { { 0, 0 }, { renderExtent.width, renderExtent.height } };
		cmd.setScissor(0, 1, &scissor);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_forwardPipelineLayout.get(), 0, 1, &GetCurrentFrameData().globalDescriptor, 0, nullptr);

		cmd.bindVertexBuffers(0, m_staticRenderData.combinedMeshBuffer.VertexBuffer().buffer, { 0 });
		cmd.bindIndexBuffer(m_staticRenderData.combinedMeshBuffer.IndexBuffer().buffer, 0, vk::IndexType::eUint32);

		vk::DeviceSize indirectOffset = 0;
		uint32_t drawStride = sizeof(vk::DrawIndexedIndirectCommand);

		DrawIndexedIndirectCommand(cmd, GetCurrentFrameData().indirectBuffer.buffer, indirectOffset, GetCurrentFrameData().drawCount, drawStride);
	}

	void VKRenderSystem::DrawIndexedIndirectCommand(vk::CommandBuffer& cmd, vk::Buffer& indirectBuffer, vk::DeviceSize offset,
		uint32_t drawCount, uint32_t stride)
	{
		cmd.drawIndexedIndirect(indirectBuffer, offset, GetCurrentFrameData().drawCount, stride);
		m_drawCalls++;
	}

	vk::CommandBuffer VKRenderSystem::RecordCopyCommandBuffer(uint32_t swapchainIdx)
	{
		vk::CommandBuffer cmd = GetCurrentFrameData().copyCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();		

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr };

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		// Setup pipeline barriers for transitioning image layouts

		vk::ImageSubresourceRange imageSubresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		// Offscreen Transition
		vk::ImageMemoryBarrier offscreenMemoryBarrier = { vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,{}, {},
				m_offscreenData.allocImages[swapchainIdx].image, imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, 
		{}, 0, nullptr, 0, nullptr, 
		1, &offscreenMemoryBarrier);

		// Swapchain Transition
		vk::ImageMemoryBarrier swapchainMemoryBarrier = { vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,{}, {},
				m_swapchainData.images[swapchainIdx], imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			{}, 0, nullptr, 0, nullptr,
			1, & swapchainMemoryBarrier);

		// Blit (Copy with auto format coversion (RGB to BGR)) offscreen to swapchain image
		vk::Offset3D blitSize =
		{
			static_cast<int32_t>(m_offscreenData.extent.width),
			static_cast<int32_t>(m_offscreenData.extent.height),
			1
		};

		std::array<vk::Offset3D, 2> offsets = {};
		offsets[1] = blitSize;

		vk::ImageBlit imageBlitRegion =
		{
			{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, offsets,
			{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, offsets
		};

		cmd.blitImage(m_offscreenData.allocImages[swapchainIdx].image, vk::ImageLayout::eTransferSrcOptimal,
			m_swapchainData.images[swapchainIdx], vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eNearest);

		// Setup pipeline barriers for transitioning image layouts back to default

		// Offscreen Transition
		offscreenMemoryBarrier = { vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eNone,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,{}, {},
				m_offscreenData.allocImages[swapchainIdx].image, imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			{}, 0, nullptr, 0, nullptr,
			1, &offscreenMemoryBarrier);

		// Swapchain Transition
		swapchainMemoryBarrier = { vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,{}, {},
				m_swapchainData.images[swapchainIdx], imageSubresourceRange };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			{}, 0, nullptr, 0, nullptr,
			1, &swapchainMemoryBarrier);

		cmd.end();

		return cmd;
	}

	vk::CommandBuffer VKRenderSystem::RecordImGuiCommandBuffer(uint32_t swapchainIdx, const vk::Extent2D& renderExtent, vk::Framebuffer framebuffer)
	{
		vk::CommandBuffer cmd = GetCurrentFrameData().imguiCommandBuffer;

		// Reset command buffer for recording new commands
		cmd.reset();

		// Begin command buffer execution
		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr, nullptr };

		VK_CHECK(cmd.begin(&cmdBeginInfo));

		vk::ClearValue clearValue;
		clearValue.color = { 1.0f, 1.0f, 1.0f, 1.0f };

		std::array<vk::ClearValue, 1> clearValues = { clearValue };

		// Begin main renderpass
		vk::RenderPassBeginInfo rpInfo = { m_renderPassImGui, framebuffer,
			vk::Rect2D{ {0, 0}, renderExtent }, clearValues.size(), clearValues.data(), nullptr };

		cmd.beginRenderPass(&rpInfo, vk::SubpassContents::eInline);

		// Record Imgui Draw Data and draw functions into command buffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		cmd.endRenderPass();

		cmd.end();

		return cmd;
	}

	void VKRenderSystem::RecordAndSubmitCommands(uint32_t swapchainIdx)
	{
		// Record command buffers
		vk::CommandBuffer mainCmd = RecordMainCommandBuffer(swapchainIdx, m_offscreenData.extent, m_offscreenData.allocImages[swapchainIdx], m_offscreenData.allocDepthImage);

		// Submit all commands
		std::vector<vk::CommandBuffer> commands = { mainCmd };

		// Prepare submission to queue
		vk::PipelineStageFlags waitStage = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo renderSubmit =
		{
			1, &GetCurrentFrameData().presentSemaphore,
			&waitStage, static_cast<uint32_t>(commands.size()), commands.data(),
			1, &GetCurrentFrameData().renderSemaphore, nullptr
		};

		std::vector submits = { renderSubmit };

		if (m_engine->shouldRenderEditorUi())
		{
			vk::CommandBuffer imguiCmd = RecordImGuiCommandBuffer(swapchainIdx, m_swapchainData.extent, m_swapchainData.framebuffers[swapchainIdx]);

			vk::SubmitInfo imguiSubmit =
			{
				1, &GetCurrentFrameData().renderSemaphore,
				&waitStage, 1, &imguiCmd,
				1, &GetCurrentFrameData().imguiSemaphore, nullptr
			};

			submits.push_back(imguiSubmit);
		}
		else
		{
			vk::CommandBuffer copyCmd = RecordCopyCommandBuffer(swapchainIdx);

			vk::SubmitInfo copySubmit =
			{
				1, &GetCurrentFrameData().renderSemaphore,
				&waitStage, 1, &copyCmd,
				1, &GetCurrentFrameData().copySemaphore, nullptr
			};

			submits.push_back(copySubmit);
		}

		VK_CHECK(m_graphicsQueue.submit(submits.size(), submits.data(), GetCurrentFrameData().renderFence));

		vk::Semaphore waitSemaphore;

		if (m_engine->shouldRenderEditorUi())
		{
			waitSemaphore = GetCurrentFrameData().imguiSemaphore;
		}
		else
		{
			waitSemaphore = GetCurrentFrameData().copySemaphore;
		}

		vk::PresentInfoKHR presentInfo =
		{
			1, &waitSemaphore, 1, &m_swapchainData.swapchain, &swapchainIdx
		};

		VK_CHECK(m_graphicsQueue.presentKHR(&presentInfo));
	}

	void VKRenderSystem::BuildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, glm::mat4& outModel) const
	{
		// Set Translation
		outModel = glm::translate(glm::mat4(1.0f), (glm::vec3)position);

		// Set Rotation
		outModel = glm::rotate(outModel, rotation.z, glm::vec3(0.0f, 0.0f, -1.0f));
		outModel = glm::rotate(outModel, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		outModel = glm::rotate(outModel, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));

		// Set Scale
		outModel = glm::scale(outModel, (glm::vec3)scale);
	}

	bool VKRenderSystem::LoadMesh(UUID meshID, MeshData& meshData)
	{
		const auto meshAsset = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshID));

		if (meshAsset && meshAsset->load())
		{
			meshData.assetID = meshID;

			meshData.numVertices = meshAsset->numVertices();
			meshData.numIndices = meshAsset->numIndices();

			meshData.vertexBuffer = Util::InitVertexBuffer(shared_from_this(), meshAsset->vertices().data(),
				meshAsset->numVertices(), meshAsset->vertexSize());

			meshData.indexBuffer = Util::InitIndexBuffer(shared_from_this(), meshAsset->indices().data(),
				meshAsset->numIndices(), meshAsset->indexSize());

			meshAsset->unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::UnloadMesh(MeshData& meshData) const
	{
		m_allocator.destroyBuffer(meshData.vertexBuffer.buffer, meshData.vertexBuffer.allocation);
		m_allocator.destroyBuffer(meshData.indexBuffer.buffer, meshData.indexBuffer.allocation);
	}

	bool VKRenderSystem::LoadTexture(UUID texID, TextureData& texData)
	{
		const auto texAsset = std::static_pointer_cast<assets::TextureAsset>(assets::AssetRegistry::get()->getAsset(texID));

		if (texAsset && texAsset->load())
		{
			texData.assetID = texID;

			texData.sampler = m_staticRenderData.textureSampler;

			texData.texture = Util::InitTexture(shared_from_this(), texAsset->pixelData(), 
				texAsset->textureWidth(), texAsset->textureHeight(), 
				texAsset->textureSizePerPixel(), g_texFormatMap.at(texAsset->textureFormat()));

			texAsset->unload();

			return true;
		}
		else
		{
			return false;
		}
	}

	void VKRenderSystem::UnloadTexture(TextureData& texData) const
	{
		m_device.destroyImageView(texData.texture.imageView);
		m_allocator.destroyImage(texData.texture.image, texData.texture.allocation);
	}

	void VKRenderSystem::BuildTextureDescriptorInfo(PackedVector<TextureData>& texData, std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(m_texData.Size());

		int idx = 0;
		for (auto& texData : texData)
		{
			vk::DescriptorImageInfo textureImageInfo = { texData.sampler, texData.texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}
}
