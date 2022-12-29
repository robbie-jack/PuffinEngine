#include "Rendering/Vulkan/VulkanRenderSystem.h"

#include <Rendering/Vulkan/Helpers/VKInitializers.h>
#include <Rendering/Vulkan/Helpers/VKDescriptors.h>
#include <Rendering/Vulkan/Helpers/VKTexture.h>
#include <Rendering/Vulkan/Helpers/VKDebug.h>

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"

#include "Types/ComponentFlags.h"

#include "MathHelpers.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

//#include "renderdoc_app.h"
#include <Rendering/Vulkan/DebugDraw.h>

#include <iostream>
#include <string>
#include <algorithm>

#include "Components/Physics/VelocityComponent.hpp"
#include "Components/Rendering/CameraComponent.h"
#include "Components/Procedural/ProceduralMeshComponent.hpp"
#include "Window/WindowSubsystem.hpp"

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err < 0)                                               \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace Puffin
{
	namespace Rendering
	{
		void VulkanRenderSystem::Init()
		{
			m_uiManager = m_engine->GetUIManager();
			m_inputSubsystem = m_engine->GetSubsystem<Input::InputSubsystem>();

			windowExtent.width = WIDTH;
			windowExtent.height = HEIGHT;

			// Initialize Offscreen Variables with Default Values
			m_offscreenExtent.width = 1024;
			m_offscreenExtent.height = 1024;
			offscreenFormat = VK_FORMAT_R8G8B8A8_SRGB;

			// Initialize Camera Variables
			editorCamera.position = glm::vec3(0.0f, 0.0f, 15.0f);
			editorCamera.aspect = (float)m_offscreenExtent.width / (float)m_offscreenExtent.height;
			InitEditorCamera();

			m_viewportSize = ImVec2(0.0f, 0.0f);
			m_shadowmapsNeedsUpdated = false;

			// Initialize Shadowmap Resolution/Format
			shadowFormat = VK_FORMAT_D16_UNORM;

			auto windowSubsystem = m_engine->GetSubsystem<Window::WindowSubsystem>();
			window = windowSubsystem->GetPrimaryWindow();

			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
			glfwMaximizeWindow(window);

			// Load Core Vulkan Structures
			InitVulkan();

			// Create Swapchain
			InitSwapchain();
			UpdateSwapchainImageViews();

			// Create Offscreen Variables
			InitOffscreenAttachments();

			// Initialize Command Pool and buffer
			InitCommands();

			// Initialise Default Renderpass
			InitDefaultRenderpass();

			// Initialize GUI Renderpass
			InitGUIRenderpass();

			// Initialize Shadow Renderpass
			InitShadowRenderPass();

			// Initialise Swapchain Framebuffers
			InitSwapchainFramebuffers();

			// Initialize Offscreen Framebuffers
			InitOffscreenFramebuffers();

			// Initialise Semaphores and Fences
			InitSyncStructures();

			InitBuffers();
			InitSceneBuffers();

			// Initialize Descriptor Sets
			InitDescriptors();

			//InitShadowPipeline();

			//InitDebugPipeline();

			InitTextureSampler();

			InitDepthSampler();

			// Initialize All Scene Objects
			PreStart();

			// Setup Forward Renderer
			m_forwardRenderer = std::make_shared<VKForwardRenderer>();
			m_forwardRenderer->Setup(shared_from_this());

			// Setup Deferred Renderer
			//SetupDeferredRenderer();

			// Pass Camera to UI
			m_uiManager->GetWindowSettings()->SetCamera(&editorCamera);

			// Subscribe to events
			m_inputEvents = std::make_shared<RingBuffer<Input::InputEvent>>();
			m_drawLineEvents = std::make_shared<RingBuffer<Debug::Line>>();
			m_drawBoxEvents = std::make_shared<RingBuffer<Debug::Box>>();

			auto eventSubsystem = m_engine->GetSubsystem<Core::EventSubsystem>();

			eventSubsystem->RegisterEvent<Debug::Line>();
			eventSubsystem->RegisterEvent<Debug::Box>();

			eventSubsystem->Subscribe<Input::InputEvent>(m_inputEvents);
			eventSubsystem->Subscribe<Debug::Line>(m_drawLineEvents);
			eventSubsystem->Subscribe<Debug::Box>(m_drawBoxEvents);

			// Initialize ImGui
			InitImGui();
			InitImguiTextures();

			m_uiManager->GetWindowViewport()->SetTextureSampler(textureSampler);

			isInitialized = true;
		}

		void VulkanRenderSystem::PreStart()
		{
			if (m_needsStarted)
			{
				InitScene();
				InitMatTextureDescriptors();
				InitShadowmapDescriptors();

				m_needsStarted = false;
			}
		}

		void VulkanRenderSystem::InitVulkan()
		{
			vkb::InstanceBuilder builder;

			// Init Vulkan Instance
			auto inst_ret = builder.set_app_name("Puffin Engine")
				.request_validation_layers(enableValidationLayers)
				.require_api_version(1, 3, 0)
				.use_default_debug_messenger()
				.build();

			vkb::Instance vkb_inst = inst_ret.value();

			// Store Instance
			instance = vkb_inst.instance;

			// Store Debug Messenger
			debug_messenger = vkb_inst.debug_messenger;

			// Get Surface of window opened by GLFW
			glfwCreateWindowSurface(instance, window, nullptr, &surface);

			// Specify Desired Device Features
			VkPhysicalDeviceFeatures supportedFeatures = {};
			supportedFeatures.samplerAnisotropy = VK_TRUE;
			supportedFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
			supportedFeatures.multiDrawIndirect = VK_TRUE;

			// Select GPU with VK Bootstrap
			// We want a gpu which can write to glfw surface and supports vulkan 1.2
			vkb::PhysicalDeviceSelector selector{ vkb_inst };
			vkb::PhysicalDevice vkbPhysicalDevice = selector
				.set_minimum_version(1, 3)
				.set_surface(surface)
				.set_required_features(supportedFeatures)
				.add_desired_extension("VK_EXT_debug_utils")
				.select()
				.value();

			// Setup VKDebug Function Pointers
			VKDebug::Setup(instance);

			VkPhysicalDeviceDescriptorIndexingFeatures supportedDescriptorFeatures = {};
			supportedDescriptorFeatures.runtimeDescriptorArray = VK_TRUE;

			/*VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features = {};
			physicalDeviceVulkan11Features.shaderDrawParameters = VK_TRUE;

			VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features = {};
			physicalDeviceVulkan12Features.runtimeDescriptorArray = VK_TRUE;*/

			// Create Final Vulkan Device
			vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
			vkb::Device vkbDevice = deviceBuilder
				.add_pNext(&supportedDescriptorFeatures)
				/*.add_pNext(&physicalDeviceVulkan11Features)
				.add_pNext(&physicalDeviceVulkan12Features)*/
				.build()
				.value();

			// Get VKDevice handle used in rest of vulkan application
			m_device = vkbDevice.device;
			physicalDevice = vkbPhysicalDevice.physical_device;

			// Use vkbootstrap to get a graphics queue
			graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
			graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

			// Initialize Memory Allocator
			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = physicalDevice;
			allocatorInfo.device = m_device;
			allocatorInfo.instance = instance;

			// Create Allocator
			vmaCreateAllocator(&allocatorInfo, &m_allocator);
		}

		void VulkanRenderSystem::InitSwapchain()
		{
			vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, m_device, surface };

			m_vkbSwapchain = {};
			m_vkbSwapchain = swapchainBuilder
				.set_old_swapchain(m_oldSwapchain)
				.use_default_format_selection()
				// VK_PRESENT_MODE_FIFO_KHR for double buffering, VK_PRESENT_MODE_MAILBOX_KHR for Triple buffering
				.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
				.set_desired_extent(windowExtent.width, windowExtent.height)
				.build()
				.value();

			// Store Swapchain and related images/views
			m_swapchain = m_vkbSwapchain.swapchain;

			swapchainImageFormat = m_vkbSwapchain.image_format;
		}

		void VulkanRenderSystem::UpdateSwapchainImageViews()
		{
			m_swapchainData.resize(m_vkbSwapchain.image_count);

			for (int i = 0; i < m_swapchainData.size(); i++)
			{
				UpdateSwapchainImageView(i);
			}
		}

		void VulkanRenderSystem::UpdateSwapchainImageView(uint32_t index)
		{
			AllocatedImage& swapchainAttachment = m_swapchainData[index].attachment;
			swapchainAttachment.image = m_vkbSwapchain.get_images().value()[INDEXID_OBJECT];

			// Set Debug Name for RenderDoc
			std::string string = "Swapchain Image " + std::to_string(index);

			VKDebug::SetObjectName(m_device,
				(uint64_t)swapchainAttachment.image,
				VK_OBJECT_TYPE_IMAGE,
				string.c_str());

			swapchainAttachment.imageView = m_vkbSwapchain.get_image_views().value()[index];
		}

		void VulkanRenderSystem::InitOffscreenAttachments()
		{
			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = m_swapchainData.size();

			m_offscreenData.resize(swapchain_imagecount);

			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				InitOffscreenAttachment(i);
				InitOffscreenDepthAttachment(i);
			}
		}

		void VulkanRenderSystem::InitOffscreenAttachment(uint32_t index)
		{
			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = m_swapchainData.size();

			// Create Images/Views for Offscreen Rendering
			VkExtent3D imageExtent =
			{
				m_offscreenExtent.width,
				m_offscreenExtent.height,
				1
			};

			// Init Image/Allocation Info
			VkImageCreateInfo imageInfo = VKInit::ImageCreateInfo(offscreenFormat, 
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
			VmaAllocationCreateInfo imageAllocInfo = {};
			imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			AllocatedImage& offscreenAttachment = m_offscreenData[index].attachment;

			// Create Image
			VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &imageAllocInfo,
				&offscreenAttachment.image, &offscreenAttachment.allocation, nullptr));

			// Create Image View
			VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(offscreenFormat,
				offscreenAttachment.image, VK_IMAGE_ASPECT_COLOR_BIT);

			VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &offscreenAttachment.imageView));

			// Set Debug Name for RenderDoc
			std::string string = "Offscreen Framebuffer " + std::to_string(index);

			VKDebug::SetObjectName(m_device,
				(uint64_t)offscreenAttachment.image,
				VK_OBJECT_TYPE_IMAGE,
				string.c_str());
		}

		void VulkanRenderSystem::InitOffscreenDepthAttachment(uint32_t index)
		{
			// Create Depth Image

			// Depth image size will match window
			VkExtent3D depthImageExtent =
			{
				m_offscreenExtent.width,
				m_offscreenExtent.height,
				1
			};

			// Hardcode depth format to 32 bit float
			depthFormat = VK_FORMAT_D32_SFLOAT;

			// Depth image will use format we selected and depth attachment usage flag
			VkImageCreateInfo depthImageInfo = VKInit::ImageCreateInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

			// Allocate depth image from local gpu memory
			VmaAllocationCreateInfo depthImageAllocInfo = {};
			depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			AllocatedImage& depthAttachment = m_offscreenData[index].depthAttachment;

			// Allocate and create image
			vmaCreateImage(m_allocator, &depthImageInfo, &depthImageAllocInfo, &depthAttachment.image, &depthAttachment.allocation, nullptr);

			// Set Debug Name for RenderDoc
			std::string string = "Offscreen Depth Image " + std::to_string(index);

			VKDebug::SetObjectName(m_device,
				(uint64_t)depthAttachment.image,
				VK_OBJECT_TYPE_IMAGE,
				string.c_str());

			// Build Image View for depth image to use in rendering
			VkImageViewCreateInfo depthImageViewInfo = VKInit::ImageViewCreateInfo(depthFormat, depthAttachment.image, VK_IMAGE_ASPECT_DEPTH_BIT);

			VK_CHECK(vkCreateImageView(m_device, &depthImageViewInfo, nullptr, &depthAttachment.imageView));
		}

		void VulkanRenderSystem::InitCommands()
		{
			// Create Command Pool for commands submitted to graphics queue
			// we also want the pool to allow for resetting individual command buffers
			VkCommandPoolCreateInfo commandPoolInfo = VKInit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			// Allocate Command Pool/Buffer for each frame data struct
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Allocate Command Pools
				VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].commandPool));

				// Allocate Default GUI and Shadow Command Buffer that we will use for scene rendering
				VkCommandBufferAllocateInfo allocInfo = VKInit::CommandBufferAllocateInfo(m_frames[i].commandPool, 1); 
				VkCommandBufferAllocateInfo allocInfoGui = VKInit::CommandBufferAllocateInfo(m_frames[i].commandPool, 1);
				VkCommandBufferAllocateInfo allocInfoShadow = VKInit::CommandBufferAllocateInfo(m_frames[i].commandPool, 1);

				// Allocate buffers
				VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &m_frames[i].mainCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfoGui, &m_frames[i].guiCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfoShadow, &m_frames[i].shadowCommandBuffer));

				// Push destruction of both command pools/buffers to deletion queue
				mainDeletionQueue.push_function([=]() 
				{
					vkDestroyCommandPool(m_device, m_frames[i].commandPool, nullptr);
				});
			}

			// Create Upload Command Pool
			VkCommandPoolCreateInfo uploadCommandPoolInfo = VKInit::CommandPoolCreateInfo(graphicsQueueFamily);

			VK_CHECK(vkCreateCommandPool(m_device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyCommandPool(m_device, uploadContext.commandPool, nullptr);
			});
		}

		void VulkanRenderSystem::InitDefaultRenderpass()
		{
			// Renderpass will use this color attachment
			VkAttachmentDescription colorAttachment = {};
			// attachment will have the format needed by the swapchain
			colorAttachment.format = offscreenFormat; // Will want to replace this with the offscreen image format later
			// 1 sample, we won't be doing msaa
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			// we clear when this attachment is loaded
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			// keep the attachment stored when renderpass ends
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			// don't care about stencil
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			// don't know or care about started layout of the attachment
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// after renderpass ends, the image has to be on a layout ready for being drawn
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference color_attachment_ref = {};
			// attachment number will index into pAttachments array in parent renderpass itself
			color_attachment_ref.attachment = 0;
			color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Create Depth Attachment
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.flags = 0;
			depthAttachment.format = depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// We are goin to create 1 subpass, which is the minimum you can do
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment_ref;
			// hook depth attachment into subpass
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create Attachments Array
			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

			// Create Renderpass
			VkRenderPassCreateInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			// connect color attachment to info
			render_pass_info.attachmentCount = attachments.size();
			render_pass_info.pAttachments = attachments.data();
			// connect subpass to info
			render_pass_info.subpassCount = 1;
			render_pass_info.pSubpasses = &subpass;
			render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
			render_pass_info.pDependencies = dependencies.data();

			VK_CHECK(vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_renderPass));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(m_device, m_renderPass, nullptr);
			});
		}

		void VulkanRenderSystem::InitGUIRenderpass()
		{
			// Create Color Attachment
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = swapchainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear screen when attachment is loaded
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep attachment stored when renderpass ends
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about initial format
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Final Image layout should be ready for displaying

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout will be optimal for drawing to

			// Create Subpass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			// Create Subpass Dependancy
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			// Create Renderpass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			// Connect color attachment
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			
			// Connect Subpass
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPassGUI));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(m_device, renderPassGUI, nullptr);
			});
		}

		void VulkanRenderSystem::InitShadowRenderPass()
		{
			// Create Depth Attachment
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = shadowFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			depthAttachmentRef.attachment = 0;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.colorAttachmentCount = 0;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			// Create Subpass dependencies for transition layouts
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Create Render Pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &depthAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPassShadows));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(m_device, renderPassShadows, nullptr);
			});
		}

		void VulkanRenderSystem::InitSwapchainFramebuffers()
		{
			// Create Framebuffers for swapchain images.
			// This will connect render pass to images for rendering

			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < m_swapchainData.size(); i++)
			{
				InitSwapchainFramebuffer(i);
			}
		}

		void VulkanRenderSystem::InitSwapchainFramebuffer(uint32_t index)
		{
			// Create Info for Swapchain Framebuffers
			VkFramebufferCreateInfo fb_gui_info = {};
			fb_gui_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_gui_info.pNext = nullptr;

			fb_gui_info.renderPass = renderPassGUI;
			fb_gui_info.width = windowExtent.width;
			fb_gui_info.height = windowExtent.height;
			fb_gui_info.layers = 1;

			// Attach swapchain image view to Framebuffer
			fb_gui_info.pAttachments = &m_swapchainData[index].attachment.imageView;
			fb_gui_info.attachmentCount = 1;

			VK_CHECK(vkCreateFramebuffer(m_device, &fb_gui_info, nullptr, &m_swapchainData[index].framebuffer));
		}

		void VulkanRenderSystem::InitOffscreenFramebuffers()
		{
			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < m_offscreenData.size(); i++)
			{
				InitOffscreenFramebuffer(i);
			}
		}

		void VulkanRenderSystem::InitOffscreenFramebuffer(uint32_t index)
		{
			// Create Info for Offscreen Framebuffers
			VkFramebufferCreateInfo fb_offscreen_info = {};
			fb_offscreen_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_offscreen_info.pNext = nullptr;

			fb_offscreen_info.renderPass = m_renderPass;
			fb_offscreen_info.width = m_offscreenExtent.width;
			fb_offscreen_info.height = m_offscreenExtent.height;
			fb_offscreen_info.layers = 1;

			AllocatedImage& offscreenAttachment = m_offscreenData[index].attachment;
			AllocatedImage& offscreenDepthAttachment = m_offscreenData[index].depthAttachment;
			VkFramebuffer& offscreenFramebuffer = m_offscreenData[index].framebuffer;

			// Attach offscreen and depth image view to Framebuffer
			VkImageView attachments[2];
			attachments[0] = offscreenAttachment.imageView;
			attachments[1] = offscreenDepthAttachment.imageView;

			fb_offscreen_info.pAttachments = attachments;
			fb_offscreen_info.attachmentCount = 2;

			VK_CHECK(vkCreateFramebuffer(m_device, &fb_offscreen_info, nullptr, &offscreenFramebuffer));
		}

		void VulkanRenderSystem::InitSyncStructures()
		{
			// Create Syncronization Structures
			// We want to create fence with Create Signaled flag
			// so we can waut on it before using
			VkFenceCreateInfo fenceCreateInfo = VKInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

			// Semaphores don't need any flags
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].renderFence));

				//enqueue the destruction of the fence
				mainDeletionQueue.push_function([=]() {
					vkDestroyFence(m_device, m_frames[i].renderFence, nullptr);
				});

				VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].shadowmapSemaphore));
				VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].presentSemaphore));
				VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore));

				//enqueue the destruction of semaphores
				mainDeletionQueue.push_function([=]() {
					vkDestroySemaphore(m_device, m_frames[i].presentSemaphore, nullptr);
					vkDestroySemaphore(m_device, m_frames[i].renderSemaphore, nullptr);
				});
			}

			// Create Upload Fence
			VkFenceCreateInfo uploadCreateFenceInfo = VKInit::FenceCreateInfo();

			VK_CHECK(vkCreateFence(m_device, &uploadCreateFenceInfo, nullptr, &uploadContext.uploadFence));

			// enqueue destruction of upload fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(m_device, uploadContext.uploadFence, nullptr);
			});
		}

		void VulkanRenderSystem::InitBuffers()
		{
			// Create Buffers for each Frame's Data
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Camera View/Proj Buffer
				m_frames[i].cameraViewProjBuffer = CreateBuffer(sizeof(GPUCameraData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Create Object Storage Buffers
				
				m_frames[i].objectBuffer = CreateBuffer(sizeof(GPUObjectData) * MAX_OBJECTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				m_frames[i].instanceBuffer = CreateBuffer(sizeof(GPUInstanceData) * MAX_INSTANCES,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Create Light Space Buffer for Shadow Vertex Stage
				m_frames[i].lightSpaceBuffer = CreateBuffer(sizeof(GPULightSpaceData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Debug Buffers
				m_frames[i].debugVertexBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * MAX_VERTICES_PER_COMMAND * sizeof(Vertex_PNTV_32),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				m_frames[i].debugIndexBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * MAX_INDICES_PER_COMMAND * sizeof(uint32_t),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				m_frames[i].debugIndirectCommandsBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * sizeof(VkDrawIndexedIndirectCommand),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU);

				m_frames[i].debugVertices.reserve(MAX_DEBUG_COMMANDS * MAX_VERTICES_PER_COMMAND);
				m_frames[i].debugIndices.reserve(MAX_DEBUG_COMMANDS * MAX_INDICES_PER_COMMAND);
			}
		}

		void VulkanRenderSystem::InitSceneBuffers()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Model Data
				m_frames[i].drawBatch.drawIndirectCommandsBuffer = CreateBuffer(MAX_OBJECTS * sizeof(VkDrawIndexedIndirectCommand),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Camera/Debug Buffer
				m_frames[i].cameraShadingBuffer = CreateBuffer(sizeof(GPUShadingData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Light Buffers
				m_frames[i].lightBuffer = CreateBuffer(sizeof(GPULightData) * MAX_LIGHTS_PER_TYPE * 3,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				m_frames[i].pointLightBuffer = CreateBuffer(sizeof(GPUPointLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				m_frames[i].dirLightBuffer = CreateBuffer(sizeof(GPUDirLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				m_frames[i].spotLightBuffer = CreateBuffer(sizeof(GPUSpotLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Light Stats Buffer
				m_frames[i].lightStatsBuffer = CreateBuffer(sizeof(GPULightStatsData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			}

			// Merged Vertex/Index Buffers
			m_sceneRenderData.mergedVertexBuffer = CreateBuffer(m_sceneRenderData.vertexBufferSize * sizeof(Vertex_PNTV_32),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

			m_sceneRenderData.mergedIndexBuffer = CreateBuffer(m_sceneRenderData.indexBufferSize * sizeof(uint32_t),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		}

		void VulkanRenderSystem::InitDescriptors()
		{
			// Initialize Descriptor Abstractions
			descriptorAllocator = new VKUtil::DescriptorAllocator{};
			descriptorAllocator->Init(m_device);

			descriptorLayoutCache = new VKUtil::DescriptorLayoutCache{};
			descriptorLayoutCache->Init(m_device);

			InitCameraVPDescriptors();
			InitObjectInstanceDescriptors();
			InitCameraShadingDescriptors();
			InitLightDataDescriptors();
			InitLightSpaceDescriptors();
		}

		void VulkanRenderSystem::InitCameraVPDescriptors()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Allocate Camera View/Proj Descriptor Set
				VkDescriptorBufferInfo cameraBufferInfo;
				cameraBufferInfo.buffer = m_frames[i].cameraViewProjBuffer.buffer;
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(GPUCameraData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(m_frames[i].cameraViewProjDescriptor, m_cameraViewProjSetLayout);
			}
		}

		void VulkanRenderSystem::InitObjectInstanceDescriptors()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Allocate Object Descriptor Set
				VkDescriptorBufferInfo objectBufferInfo;
				objectBufferInfo.buffer = m_frames[i].objectBuffer.buffer;
				objectBufferInfo.offset = 0;
				objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

				// Allocated Instance Descriptor Set
				VkDescriptorBufferInfo instanceBufferInfo;
				instanceBufferInfo.buffer = m_frames[i].instanceBuffer.buffer;
				instanceBufferInfo.offset = 0;
				instanceBufferInfo.range = sizeof(GPUInstanceData) * MAX_INSTANCES;

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.BindBuffer(1, &instanceBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(m_frames[i].objectInstanceDescriptor, m_objectInstanceSetLayout);
			}
		}

		void VulkanRenderSystem::InitCameraShadingDescriptors()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Camera/Debug Shading Info
				VkDescriptorBufferInfo cameraShadingInfo;
				cameraShadingInfo.buffer = m_frames[i].cameraShadingBuffer.buffer;
				cameraShadingInfo.offset = 0;
				cameraShadingInfo.range = sizeof(GPUShadingData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &cameraShadingInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(m_frames[i].cameraShadingDescriptor, m_cameraShadingSetLayout);
			}
		}

		void VulkanRenderSystem::InitLightDataDescriptors()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Light Info
				VkDescriptorBufferInfo lightInfo;
				lightInfo.buffer = m_frames[i].lightBuffer.buffer;
				lightInfo.offset = 0;
				lightInfo.range = sizeof(GPULightData) * MAX_LIGHTS_PER_TYPE * 3;

				VkDescriptorBufferInfo pointLightInfo;
				pointLightInfo.buffer = m_frames[i].pointLightBuffer.buffer;
				pointLightInfo.offset = 0;
				pointLightInfo.offset = 0;
				pointLightInfo.range = sizeof(GPUPointLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo dirLightInfo;
				dirLightInfo.buffer = m_frames[i].dirLightBuffer.buffer;
				dirLightInfo.offset = 0;
				dirLightInfo.range = sizeof(GPUDirLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo spotLightInfo;
				spotLightInfo.buffer = m_frames[i].spotLightBuffer.buffer;
				spotLightInfo.offset = 0;
				spotLightInfo.range = sizeof(GPUSpotLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo lightStatsInfo;
				lightStatsInfo.buffer = m_frames[i].lightStatsBuffer.buffer;
				lightStatsInfo.offset = 0;
				lightStatsInfo.range = sizeof(GPULightStatsData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &lightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(1, &pointLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(2, &dirLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(3, &spotLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(4, &lightStatsInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(m_frames[i].lightDataDescriptor, m_lightDataSetLayout);
			}
		}

		void VulkanRenderSystem::InitLightSpaceDescriptors()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				VkDescriptorBufferInfo lightSpaceInfo;
				lightSpaceInfo.buffer = m_frames[i].lightSpaceBuffer.buffer;
				lightSpaceInfo.offset = 0;
				lightSpaceInfo.range = sizeof(GPULightSpaceData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &lightSpaceInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(m_frames[i].lightSpaceDescriptor, m_lightSpaceSetLayout);
			}
		}

		void VulkanRenderSystem::InitShadowPipeline()
		{
			// Read Shader Code from files
			const fs::path shaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\shadowmap_vert.spv";
			auto vertShaderCode = ReadFile(shaderPath.string());

			// Create Shader Module from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(m_device, vertShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_lightSpaceSetLayout,
				m_objectInstanceSetLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout));

			// Create Pipeline Builder Object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));

			VkVertexInputBindingDescription bindingDescription;
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			Vertex_PNTV_32::GetVertexBindingAndAttributes(bindingDescription, attributeDescriptions);

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);

			// Multisampling
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			VkPipelineColorBlendAttachmentState colorBlendAttachment = VKInit::ColorBlendAttachmentState();
			pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(1, &colorBlendAttachment);

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Define Viewport/Scissor as Dynamic State for Pipeline
			std::vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			// Dynamic Viewport/Scissor Size
			pipelineBuilder.dynamic = VKInit::DynamicStateCreateInfo(dynamicStates);

			// Assign Pipeline Layout
			pipelineBuilder.pipelineLayout = shadowPipelineLayout;

			// Build Pipeline
			shadowPipeline = pipelineBuilder.BuildPipeline(m_device, renderPassShadows);
		}

		void VulkanRenderSystem::InitDebugPipeline()
		{
			const fs::path vertShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\debug_vert.spv";
			const fs::path fragShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\debug_frag.spv";

			// Read Shader Code from Files
			auto vertShaderCode = ReadFile(vertShaderPath.string());
			auto fragShaderCode = ReadFile(fragShaderPath.string());

			// Create Shader Modules from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(m_device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(m_device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_cameraViewProjSetLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &debugPipelineLayout));

			// Pipeline Builder Object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			VkVertexInputBindingDescription bindingDescription;
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			Vertex_PC_32::GetVertexBindingAndAttributes(bindingDescription, attributeDescriptions);

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info - Will do line rendering her instead of filled triangle rendering
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

			// Define Viewport
			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)m_offscreenExtent.width;
			pipelineBuilder.viewport.height = (float)m_offscreenExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			// Define Scissor Extent (Pixels Outside Scissor Rectangle will be discarded)
			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = m_offscreenExtent;

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			VkPipelineColorBlendAttachmentState colorBlendAttachment = VKInit::ColorBlendAttachmentState();
			pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(1, &colorBlendAttachment);

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(false, false, VK_COMPARE_OP_LESS_OR_EQUAL);

			std::vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			// Dynamic Viewport/Scissor Size
			pipelineBuilder.dynamic = VKInit::DynamicStateCreateInfo(dynamicStates);

			// Assign Pipeline Layout to Pipeline
			pipelineBuilder.pipelineLayout = debugPipelineLayout;

			// Build Pipeline
			debugPipeline = pipelineBuilder.BuildPipeline(m_device, m_renderPass);
		}

		void VulkanRenderSystem::InitScene()
		{
			// Initialize Shadowcaster Lights
			std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
			ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
			for (const auto& entity : shadowcasterLightEntities)
			{
				InitShadowcasterLight(entity->ID());

				entity->SetComponentFlag<ShadowCasterComponent, FlagDirty>(false);
			}

			// Initialize Meshes
			std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
			ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
			for (const auto entity : meshEntities)
			{
				auto& mesh = entity->GetComponent<MeshComponent>();

				InitAlbedoTexture(mesh.textureAssetID);
				m_sceneRenderData.albedoTextureData[mesh.textureAssetID].entities.insert(entity->ID());

				InitMesh(entity->ID(), mesh.meshAssetID);

				entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
			}

			std::vector<std::shared_ptr<ECS::Entity>> proceduralMeshEntities;
			ECS::GetEntities<TransformComponent, ProceduralMeshComponent>(m_world, proceduralMeshEntities);
			for (const auto& entity : proceduralMeshEntities)
			{
				auto& mesh = entity->GetComponent<ProceduralMeshComponent>();

				InitAlbedoTexture(mesh.textureAssetID);
				m_sceneRenderData.albedoTextureData[mesh.textureAssetID].entities.insert(entity->ID());

				InitProceduralMesh(entity->ID());

				entity->SetComponentFlag<ProceduralMeshComponent, FlagDirty>(false);
			}
		}

		void VulkanRenderSystem::InitMatTextureDescriptors()
		{
			// Grab Image Views from Initialized Meshes
			std::vector<VkDescriptorImageInfo> albedoImageInfo;
			std::vector<VkDescriptorImageInfo> normalImageInfo;

			VkDescriptorImageInfo textureImageInfo;
			textureImageInfo.sampler = depthSampler;
			textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
			ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
			for (const auto& entity : meshEntities)
			{
				auto& mesh = entity->GetComponent<MeshComponent>();

				//Albedo Textures
				textureImageInfo.imageView = m_sceneRenderData.albedoTextureData[mesh.textureAssetID].texture.imageView;
				albedoImageInfo.push_back(textureImageInfo);

				// Normal Maps	
				textureImageInfo.imageView = m_sceneRenderData.albedoTextureData[mesh.textureAssetID].texture.imageView;
				normalImageInfo.push_back(textureImageInfo);
			}

			std::vector<std::shared_ptr<ECS::Entity>> proceduralMeshEntities;
			ECS::GetEntities<TransformComponent, ProceduralMeshComponent>(m_world, proceduralMeshEntities);
			for (const auto& entity : proceduralMeshEntities)
			{
				auto& mesh = entity->GetComponent<ProceduralMeshComponent>();

				//Albedo Textures
				textureImageInfo.imageView = m_sceneRenderData.albedoTextureData[mesh.textureAssetID].texture.imageView;
				albedoImageInfo.push_back(textureImageInfo);

				// Normal Maps	
				textureImageInfo.imageView = m_sceneRenderData.albedoTextureData[mesh.textureAssetID].texture.imageView;
				normalImageInfo.push_back(textureImageInfo);
			}

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				if (albedoImageInfo.size() > 0 && normalImageInfo.size() > 0)
				{
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.BindImages(0, static_cast<uint32_t>(albedoImageInfo.size()),
							albedoImageInfo.data(),
							VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.BindImages(1, static_cast<uint32_t>(normalImageInfo.size()),
							normalImageInfo.data(),
							VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.Build(m_frames[i].matTextureDescriptor, m_matTextureSetLayout);
				}
			}
		}

		void VulkanRenderSystem::InitMesh(ECS::EntityID entityID, UUID meshID)
		{
			// Setup Mesh Render Data
			if (m_sceneRenderData.meshRenderDataMap.count(meshID) == 0)
			{
				// This is the first entity to render with this mesh, create a new render data struct
				MeshRenderData meshData;

				meshData.meshAssetID = meshID;

				const auto staticMeshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshID));

				// Load Mesh Data
				if (staticMeshAsset && staticMeshAsset->Load())
				{
					meshData.vertexCount = static_cast<uint32_t>(staticMeshAsset->GetVertices().size());
					meshData.indexCount = static_cast<uint32_t>(staticMeshAsset->GetIndices().size());

					MeshBufferData bufferData;
					bufferData.vertexData = (void*)staticMeshAsset->GetVertices().data();
					bufferData.vertexSize = sizeof(Vertex_PNTV_32);
					bufferData.indexData = (void*)staticMeshAsset->GetIndices().data();
					bufferData.indexSize = sizeof(uint32_t);

					AddMeshRenderDataToScene(meshData, bufferData);
				}

				// Unload vertices from main memory
				staticMeshAsset->Unload();

				// Add mesh data to map
				m_sceneRenderData.meshRenderDataMap[meshID] = meshData;
			}

			// Increment instance count and add entity to set
			m_sceneRenderData.meshRenderDataMap[meshID].entities.insert(entityID);
		}

		void VulkanRenderSystem::InitProceduralMesh(ECS::EntityID entity)
		{
			auto& mesh = m_world->GetComponent<ProceduralMeshComponent>(entity);

			// Setup Mesh Render Data
			if (m_sceneRenderData.meshRenderDataMap.count(entity) == 0)
			{
				MeshRenderData meshData;

				meshData.meshAssetID = entity;

				// Load Mesh Data
				if (!mesh.vertices.empty() && !mesh.indices.empty())
				{
					meshData.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
					meshData.indexCount = static_cast<uint32_t>(mesh.indices.size());

					MeshBufferData bufferData;
					bufferData.vertexData = (void*)mesh.vertices.data();
					bufferData.vertexSize = sizeof(Vertex_PNTV_32);
					bufferData.indexData = (void*)mesh.indices.data();
					bufferData.indexSize = sizeof(uint32_t);

					AddMeshRenderDataToScene(meshData, bufferData);
				}

				mesh.vertices.clear();
				mesh.indices.clear();

				// Add mesh data to map
				m_sceneRenderData.meshRenderDataMap[entity] = meshData;
			}

			// Increment instance count and add entity to set
			m_sceneRenderData.meshRenderDataMap[entity].entities.insert(entity);
		}

		void VulkanRenderSystem::InitShadowcasterLight(ECS::EntityID entity)
		{
			auto& shadowcaster = m_world->GetComponent<ShadowCasterComponent>(entity);

			// Create Depth Image for Light
			VkExtent3D imageExtent =
			{
				shadowcaster.shadowmapWidth,
				shadowcaster.shadowmapHeight,
				1
			};
			
			VkImageCreateInfo imageInfo = VKInit::ImageCreateInfo(shadowFormat,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);

			VmaAllocationCreateInfo imageAllocInfo = {};
			imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Create Framebuffer Info for Light
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.pNext = nullptr;

			framebufferInfo.renderPass = renderPassShadows;
			framebufferInfo.width = shadowcaster.shadowmapWidth;
			framebufferInfo.height = shadowcaster.shadowmapHeight;
			framebufferInfo.layers = 1;

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Image/Allocation
				if (m_frames[i].shadowmapImages.count(entity) == 0)
				{
					m_frames[i].shadowmapImages[entity] = AllocatedImage();
				}

				vmaCreateImage(m_allocator, &imageInfo, &imageAllocInfo,
					&m_frames[i].shadowmapImages[entity].image,
					&m_frames[i].shadowmapImages[entity].allocation, nullptr);

				std::string string = "Light Depth Image " + std::to_string(i);

				VKDebug::SetObjectName(m_device,
					(uint64_t)m_frames[i].shadowmapImages[entity].image,
					VK_OBJECT_TYPE_IMAGE,
					string.c_str());

				// Create Image View
				VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(shadowFormat,
					m_frames[i].shadowmapImages[entity].image, VK_IMAGE_ASPECT_DEPTH_BIT);

				VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_frames[i].shadowmapImages[entity].imageView));

				// Create Framebuffer
				framebufferInfo.pAttachments = &m_frames[i].shadowmapImages[entity].imageView;
				framebufferInfo.attachmentCount = 1;

				VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frames[i].shadowmapFramebuffers[entity]));
			}
		}

		void VulkanRenderSystem::InitCamera(ECS::EntityID entity)
		{
			auto& transform = m_world->GetComponent<TransformComponent>(entity);
			auto& camera = m_world->GetComponent<CameraComponent>(entity);

			// Calculate Perspective Projection
			UpdateCameraPerspective(camera, camera.fov, camera.aspect, camera.zNear, camera.zFar);

			// Calculate Right and Up Vectors
			camera.right = glm::normalize(glm::cross(static_cast<glm::vec3>(camera.up), static_cast<glm::vec3>(camera.direction)));
			camera.up = glm::cross(static_cast<glm::vec3>(camera.direction), static_cast<glm::vec3>(camera.right));
			camera.lookat = transform.position + camera.direction;

			// Calculate Camera View Matrix
			camera.matrices.view = UpdateCameraView(transform.position, camera.lookat, camera.up);
		}

		void VulkanRenderSystem::InitEditorCamera()
		{
			// Calculate Perspective Projection
			UpdateCameraPerspective(editorCamera, editorCamera.fov, editorCamera.aspect, editorCamera.zNear, editorCamera.zFar);

			// Calculate Right and Up Vectors
			editorCamera.right = glm::normalize(glm::cross(static_cast<glm::vec3>(editorCamera.up), static_cast<glm::vec3>(editorCamera.direction)));
			editorCamera.up = glm::cross(static_cast<glm::vec3>(editorCamera.direction), static_cast<glm::vec3>(editorCamera.right));
			editorCamera.lookat = editorCamera.position + editorCamera.direction;

			// Calculate Camera View Matrix
			editorCamera.matrices.view = UpdateCameraView(editorCamera.position, editorCamera.lookat, editorCamera.up);
		}

		void VulkanRenderSystem::InitAlbedoTexture(UUID uuid)
		{
			if (m_sceneRenderData.albedoTextureData.count(uuid) == 0)
			{
				TextureRenderData data;

				const auto textureAsset = std::static_pointer_cast<Assets::TextureAsset>(Assets::AssetRegistry::Get()->GetAsset(uuid));

				if (textureAsset && textureAsset->Load())
				{
					IO::InitTextureImage(*this, textureAsset->GetPixelData(), textureAsset->GetTextureWidth(), textureAsset->GetTextureHeight(), data.texture);

					textureAsset->Unload();
				}

				VKDebug::SetObjectName(m_device,
					(uint64_t)data.texture.image,
					VK_OBJECT_TYPE_IMAGE,
					"Mesh Texture");

				VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_UNORM, data.texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
				VK_CHECK(vkCreateImageView(m_device, &imageViewInfo, nullptr, &data.texture.imageView));

				m_sceneRenderData.albedoTextureData[uuid] = data;
			}
		}

		AllocatedBuffer VulkanRenderSystem::InitVertexBuffer(const void* vertexData, const size_t verticesSize)
		{
			// Copy Loaded Mesh data into mesh vertex buffer

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(verticesSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
			memcpy(data, vertexData, verticesSize);
			vmaUnmapMemory(m_allocator, stagingBuffer.allocation);

			// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
			AllocatedBuffer vertexBuffer = CreateBuffer(verticesSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT |VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			// Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = verticesSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

			return vertexBuffer;
		}

		AllocatedBuffer VulkanRenderSystem::InitIndexBuffer(const void* indexData, const size_t indicesSize)
		{
			// Copy Loaded Index data into mesh index buffer

			// Allocated Staging Buffer - Map Indices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(indicesSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map Index data to staging buffer
			void* data;
			vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
			memcpy(data, indexData, indicesSize);
			vmaUnmapMemory(m_allocator, stagingBuffer.allocation);

			// Allocate Index Buffer - Transfer Indices into GPU Memory
			AllocatedBuffer indexBuffer = CreateBuffer(indicesSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = indicesSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, indexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer
			vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

			return indexBuffer;
		}

		void VulkanRenderSystem::CopyVerticesToBuffer(const void* vertexData, const size_t verticesSize, AllocatedBuffer vertexBuffer, uint32_t copyOffset)
		{
			// Get Size of data to be transfered to vertex buffer

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(verticesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
			memcpy(data, vertexData, verticesSize);
			vmaUnmapMemory(m_allocator, stagingBuffer.allocation);

			// Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = copyOffset;
				copy.srcOffset = 0;
				copy.size = verticesSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanRenderSystem::CopyIndicesToBuffer(const void* indexData, const size_t indicesSize, AllocatedBuffer indexBuffer, uint32_t copyOffset)
		{
			// Copy Loaded Index data into mesh index buffer

			// Allocated Staging Buffer - Map Indices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(indicesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map Index data to staging buffer
			void* data;
			vmaMapMemory(m_allocator, stagingBuffer.allocation, &data);
			memcpy(data, indexData, indicesSize);
			vmaUnmapMemory(m_allocator, stagingBuffer.allocation);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = copyOffset;
				copy.srcOffset = 0;
				copy.size = indicesSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, indexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer
			vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanRenderSystem::CleanupTexture(ECS::EntityID entityID, UUID textureID)
		{
			// Cleanup Texture
			m_sceneRenderData.albedoTextureData[textureID].entities.erase(entityID);

			if (m_sceneRenderData.albedoTextureData[textureID].entities.empty())
			{
				vmaDestroyImage(m_allocator, m_sceneRenderData.albedoTextureData[textureID].texture.image,
					m_sceneRenderData.albedoTextureData[textureID].texture.allocation);
			}
		}

		void VulkanRenderSystem::CleanupMesh(ECS::EntityID entityID, UUID meshID)
		{
			// Decrement instance count and remove entity from set
			m_sceneRenderData.meshRenderDataMap[meshID].entities.erase(entityID);
		}

		void VulkanRenderSystem::CleanupShadowcasterLight(ECS::EntityID entity)
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Destroy Image/View
				vkDestroyImageView(m_device, m_frames[i].shadowmapImages[entity].imageView, nullptr);
				vmaDestroyImage(m_allocator, m_frames[i].shadowmapImages[entity].image, m_frames[i].shadowmapImages[entity].allocation);

				m_frames[i].shadowmapImages.erase(entity);

				// Destroy Framebuffer
				vkDestroyFramebuffer(m_device, m_frames[i].shadowmapFramebuffers[entity], nullptr);

				m_frames[i].shadowmapFramebuffers.erase(entity);
			}
		}

		void VulkanRenderSystem::InitTextureSampler()
		{
			VkSamplerCreateInfo samplerInfo = VKInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;

			VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &textureSampler));
		}

		void VulkanRenderSystem::InitDepthSampler()
		{
			VkSamplerCreateInfo samplerInfo = VKInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;

			VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &depthSampler));
		}

		void VulkanRenderSystem::SetupDeferredRenderer()
		{
			std::vector<VkCommandPool> commandPools;

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				commandPools.push_back(m_frames[i].commandPool);
			}

			deferredRenderer.Setup(physicalDevice,
			                       m_device,
			                       m_allocator,
			                       descriptorAllocator,
			                       descriptorLayoutCache,
			                       commandPools,
			                       FRAME_OVERLAP,
								   m_offscreenExtent);

			deferredRenderer.SetupGeometry(m_cameraViewProjSetLayout, m_objectInstanceSetLayout, m_matTextureSetLayout);
			deferredRenderer.SetupShading(m_renderPass, m_cameraShadingSetLayout, m_lightDataSetLayout, m_shadowMapSetLayout);
		}

		void VulkanRenderSystem::InitShadowmapDescriptors()
		{
			// Write Shadowmap Descriptor Sets
			VkDescriptorImageInfo shadowmapBufferInfo;
			shadowmapBufferInfo.sampler = depthSampler;
			shadowmapBufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			// Initialise Shadowmap Descriptor
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				std::vector<VkDescriptorImageInfo> imageInfos;

				std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
				ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
				for (const auto& entity : shadowcasterLightEntities)
				{
					shadowmapBufferInfo.imageView = m_frames[i].shadowmapImages[entity->ID()].imageView;
					imageInfos.push_back(shadowmapBufferInfo);
				}

				if (shadowcasterLightEntities.empty())
				{
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.Build(m_frames[i].shadowmapDescriptor, m_shadowMapSetLayout);
				}
				else
				{
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.BindImages(0, static_cast<uint32_t>(imageInfos.size()), imageInfos.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.Build(m_frames[i].shadowmapDescriptor, m_shadowMapSetLayout);
				}
			}
		}

		void VulkanRenderSystem::UpdateShadowmapDescriptors()
		{
			// Write Shadowmap Descriptor Sets
			VkDescriptorImageInfo shadowmapBufferInfo;
			shadowmapBufferInfo.sampler = depthSampler;
			shadowmapBufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			// Initialise Shadowmap Descriptor
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				std::vector<VkDescriptorImageInfo> imageInfos;

				std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
				ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
				for (const auto& entity : shadowcasterLightEntities)
				{
					shadowmapBufferInfo.imageView = m_frames[i].shadowmapImages[entity->ID()].imageView;
					imageInfos.push_back(shadowmapBufferInfo);
				}

				if (!shadowcasterLightEntities.empty())
				{
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.UpdateImages(0, static_cast<uint32_t>(imageInfos.size()), imageInfos.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
						.Update(m_frames[i].shadowmapDescriptor);
				}
			}
		}

		void VulkanRenderSystem::InitImGui()
		{
			// Create Descriptor Pool for ImGui
			VkDescriptorPoolSize poolSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000;
			pool_info.poolSizeCount = std::size(poolSizes);
			pool_info.pPoolSizes = poolSizes;

			VkDescriptorPool imguiPool;
			VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imguiPool));


			// Initialize imgui library
			// this initializes the core structures of imgui
			//ImGui::CreateContext();

			// Initialize ImGui for GLFW
			ImGui_ImplGlfw_InitForVulkan(window, true);

			//this initializes imgui for Vulkan
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = instance;
			init_info.PhysicalDevice = physicalDevice;
			init_info.Device = m_device;
			init_info.Queue = graphicsQueue;
			init_info.DescriptorPool = imguiPool;
			init_info.MinImageCount = 3;
			init_info.ImageCount = 3;

			ImGui_ImplVulkan_Init(&init_info, renderPassGUI);

			//execute a gpu command to upload imgui font textures
			ImmediateSubmit([&](VkCommandBuffer cmd) 
			{
				ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});

			//clear font textures from cpu data
			ImGui_ImplVulkan_DestroyFontUploadObjects();

			//add the destroy the imgui created structures
			mainDeletionQueue.push_function([=]() {

				vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
				ImGui_ImplVulkan_Shutdown();
			});
		}

		void VulkanRenderSystem::InitImguiTextures()
		{
			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < m_offscreenData.size(); i++)
			{
				InitImGuiTexture(i);
			}
		}

		void VulkanRenderSystem::InitImGuiTexture(uint32_t index)
		{
			AllocatedImage& offscreenAttachment = m_offscreenData[index].attachment;

			// Create Texture ID for rendering Viewport to ImGui Window
			m_offscreenData[index].viewportTexture = ImGui_ImplVulkan_AddTexture(textureSampler, offscreenAttachment.imageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		void VulkanRenderSystem::DestroyImguiTexture(uint32_t index)
		{
			ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)m_offscreenData[index].viewportTexture);
			m_offscreenData[index].viewportTexture = nullptr;
		}

		//-------------------------------------------------------------------------------------

		void VulkanRenderSystem::UpdatSwapchainData(uint32_t index)
		{
			if (m_swapchainData[index].needsRecreated == true)
			{
				DestroySwapchainData(index);

				UpdateSwapchainImageView(index);
				InitSwapchainFramebuffer(index);

				m_swapchainData[index].needsRecreated = false;

				// Exit out of method if there is still swapchain data needing updated
				for (int i = 0; i < m_swapchainData.size(); i++)
				{
					if (m_swapchainData[i].needsRecreated == true)
						return;
				}

				// Cleanup old swapchain (only if it hasn't been reused
				if (m_oldSwapchain != m_swapchain)
				{
					vkDestroySwapchainKHR(m_device, m_oldSwapchain, nullptr);
				}
			}
		}

		void VulkanRenderSystem::DestroySwapchainData(uint32_t index)
		{
			vkDestroyFramebuffer(m_device, m_swapchainData[index].framebuffer, nullptr);
			vkDestroyImageView(m_device, m_swapchainData[index].attachment.imageView, nullptr);
		}

		void VulkanRenderSystem::UpdateOffscreenData(uint32_t index)
		{
			if (m_offscreenData[index].needsRecreated == true)
			{
				// Destroy all Offscreen Variables
				DestroyOffscreenData(index);

				// Initialize Offscreen Variables and Scene
				InitOffscreenAttachment(index);
				InitOffscreenDepthAttachment(index);
				InitOffscreenFramebuffer(index);
				InitImGuiTexture(index);

				m_offscreenData[index].needsRecreated = false;
			}
		}

		void VulkanRenderSystem::DestroyOffscreenData(uint32_t index)
		{
			vkDestroyImageView(m_device, m_offscreenData[index].attachment.imageView, nullptr);
			vmaDestroyImage(m_allocator, m_offscreenData[index].attachment.image, m_offscreenData[index].attachment.allocation);

			vkDestroyImageView(m_device, m_offscreenData[index].depthAttachment.imageView, nullptr);
			vmaDestroyImage(m_allocator, m_offscreenData[index].depthAttachment.image, m_offscreenData[index].depthAttachment.allocation);

			vkDestroyFramebuffer(m_device, m_offscreenData[index].framebuffer, nullptr);

			DestroyImguiTexture(index);
		}

		//-------------------------------------------------------------------------------------

		void VulkanRenderSystem::Update()
		{
			ProcessEvents();

			//m_uiManager->DrawUI(m_engine->GetDeltaTime());

			// Initialize/Cleanup marked components
			std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
			ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
			for (const auto& entity : meshEntities)
			{
				auto& mesh = entity->GetComponent<MeshComponent>();

				// Initialize
				if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
				{
					InitAlbedoTexture(mesh.textureAssetID);
					m_sceneRenderData.albedoTextureData[mesh.textureAssetID].entities.insert(entity->ID());

					InitMesh(entity->ID(), mesh.meshAssetID);

					entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
				}

				// Cleanup
				if (entity->GetComponentFlag<MeshComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{
					CleanupTexture(entity->ID(), mesh.textureAssetID);
					CleanupMesh(entity->ID(), mesh.meshAssetID);

					entity->RemoveComponent<MeshComponent>();
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> proceduralMeshEntities;
			ECS::GetEntities<TransformComponent, ProceduralMeshComponent>(m_world, proceduralMeshEntities);
			for (const auto& entity : proceduralMeshEntities)
			{
				auto& mesh = entity->GetComponent<ProceduralMeshComponent>();

				if (entity->GetComponentFlag<ProceduralMeshComponent, FlagDirty>())
				{
					InitAlbedoTexture(mesh.textureAssetID);
					m_sceneRenderData.albedoTextureData[mesh.textureAssetID].entities.insert(entity->ID());

					InitProceduralMesh(entity->ID());

					entity->SetComponentFlag<ProceduralMeshComponent, FlagDirty>(false);
				}

				if (entity->GetComponentFlag<ProceduralMeshComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{
					CleanupTexture(entity->ID(), mesh.textureAssetID);

					// Decrement instance count and remove entity from set
					m_sceneRenderData.meshRenderDataMap[entity->ID()].entities.erase(entity->ID());

					entity->RemoveComponent<ProceduralMeshComponent>();
				}
			}
			
			/*std::vector<std::shared_ptr<ECS::Entity>> pointLightEntities;
			ECS::GetEntities<TransformComponent, PointLightComponent>(m_world, pointLightEntities);
			for (const auto& entity : pointLightEntities)
			{
				if (entity->GetComponentFlag<PointLightComponent, FlagDirty>())
				{
					
				}

				if (entity->GetComponentFlag<PointLightComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{
					
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> dirLightEntities;
			ECS::GetEntities<TransformComponent, DirectionalLightComponent>(m_world, dirLightEntities);
			for (const auto& entity : dirLightEntities)
			{
				if (entity->GetComponentFlag<DirectionalLightComponent, FlagDirty>())
				{
					
				}

				if (entity->GetComponentFlag<DirectionalLightComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{
					
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> spotLightEntities;
			ECS::GetEntities<TransformComponent, SpotLightComponent>(m_world, spotLightEntities);
			for (const auto& entity : spotLightEntities)
			{
				if (entity->GetComponentFlag<SpotLightComponent, FlagDirty>())
				{

				}

				if (entity->GetComponentFlag<SpotLightComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{

				}
			}*/

			std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
			ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
			for (const auto& entity : shadowcasterLightEntities)
			{
				if (entity->GetComponentFlag<ShadowCasterComponent, FlagDirty>())
				{
					InitShadowcasterLight(entity->ID());

					entity->SetComponentFlag<ShadowCasterComponent, FlagDirty>(false);

					m_shadowmapsNeedsUpdated = true;
				}

				if (entity->GetComponentFlag<ShadowCasterComponent, FlagDeleted>() || entity->GetFlag<FlagDeleted>())
				{
					CleanupShadowcasterLight(entity->ID());

					entity->RemoveComponent<ShadowCasterComponent>();

					m_shadowmapsNeedsUpdated = true;
				}
			}

			std::vector<std::shared_ptr<ECS::Entity>> cameraEntities;
			ECS::GetEntities<TransformComponent, CameraComponent>(m_world, cameraEntities);
			for (const auto& entity : cameraEntities)
			{
				UpdateCamera(entity->ID());
			}

			UpdateEditorCamera();

			// Rebuild Shadowmaps if needed
			if (m_shadowmapsNeedsUpdated)
			{
				UpdateShadowmapDescriptors();
				m_shadowmapsNeedsUpdated = false;
			}

			DrawFrame();
		}

		void VulkanRenderSystem::Stop()
		{
			std::vector<std::shared_ptr<ECS::Entity>> meshEntities;
			ECS::GetEntities<TransformComponent, MeshComponent>(m_world, meshEntities);
			for (const auto& entity : meshEntities)
			{
				auto& mesh = entity->GetComponent<MeshComponent>();

				CleanupMesh(entity->ID(), mesh.meshAssetID);
			}

			std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
			ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
			for (const auto& entity : shadowcasterLightEntities)
			{
				CleanupShadowcasterLight(entity->ID());
			}

			m_needsStarted = true;
		}

		void VulkanRenderSystem::ProcessEvents()
		{
			// Process Input Events
			Input::InputEvent inputEvent;
			while (!m_inputEvents->IsEmpty())
			{
				if (m_inputEvents->Pop(inputEvent))
				{
					if (inputEvent.actionName == "CamMoveLeft")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveLeft = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveLeft = false;
						}
					}

					if (inputEvent.actionName == "CamMoveRight")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveRight = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveRight = false;
						}
					}

					if (inputEvent.actionName == "CamMoveForward")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveForward = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveForward = false;
						}
					}

					if (inputEvent.actionName == "CamMoveBackward")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveBackward = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveBackward = false;
						}
					}

					if (inputEvent.actionName == "CamMoveUp")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveUp = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveUp = false;
						}
					}

					if (inputEvent.actionName == "CamMoveDown")
					{
						if (inputEvent.actionState == Puffin::Input::KeyState::PRESSED)
						{
							moveDown = true;
						}
						else if (inputEvent.actionState == Puffin::Input::KeyState::RELEASED)
						{
							moveDown = false;
						}
					}
				}
			}

			// Process Debug Draw Events
			Debug::Line drawLineEvent;
			while (!m_drawLineEvents->IsEmpty())
			{
				if (m_drawLineEvents->Pop(drawLineEvent))
				{
					DrawDebugLine(drawLineEvent);
				}
			}

			Debug::Box drawBoxEvent;
			while (!m_drawBoxEvents->IsEmpty())
			{
				if (m_drawBoxEvents->Pop(drawBoxEvent))
				{
					DrawDebugBox(drawBoxEvent);
				}
			}
		}

		void VulkanRenderSystem::UpdateCamera(ECS::EntityID entity)
		{
			auto& transform = m_world->GetComponent<TransformComponent>(entity);
			auto& camera = m_world->GetComponent<CameraComponent>(entity);

			// Calculate Right, Up and LookAt vectors
			camera.right = camera.up.Cross(camera.direction).Normalised();
			camera.lookat = transform.position + camera.direction;

			float newAspect = (float)m_offscreenExtent.width / (float)m_offscreenExtent.height;

			// Recalculate camera perspective if fov has changed, store new fov in prevFov
			if (camera.fov != camera.prevFov || camera.aspect != newAspect)
			{
				UpdateCameraPerspective(camera, camera.fov, newAspect, camera.zNear, camera.zFar);
			}

			camera.matrices.view = UpdateCameraView(transform.position, camera.lookat, camera.up);
		}

		void VulkanRenderSystem::UpdateEditorCamera()
		{
			if (m_inputSubsystem->IsCursorLocked())
			{
				// Camera Movement
				if (moveLeft && !moveRight)
				{
					editorCamera.position += editorCamera.right * editorCamera.speed * m_engine->GetDeltaTime();
				}

				if (moveRight && !moveLeft)
				{
					editorCamera.position -= editorCamera.right * editorCamera.speed * m_engine->GetDeltaTime();
				}

				if (moveForward && !moveBackward)
				{
					editorCamera.position += editorCamera.direction * editorCamera.speed * m_engine->GetDeltaTime();
				}

				if (moveBackward && !moveForward)
				{
					editorCamera.position -= editorCamera.direction * editorCamera.speed * m_engine->GetDeltaTime();
				}

				if (moveUp && !moveDown)
				{
					editorCamera.position += editorCamera.up * editorCamera.speed * m_engine->GetDeltaTime();
				}

				if (moveDown && !moveUp)
				{
					editorCamera.position -= editorCamera.up * editorCamera.speed * m_engine->GetDeltaTime();
				}

				// Mouse Rotation
				editorCamera.yaw += m_inputSubsystem->GetMouseXOffset();
				editorCamera.pitch -= m_inputSubsystem->GetMouseYOffset();

				if (editorCamera.pitch > 89.0f)
					editorCamera.pitch = 89.0f;

				if (editorCamera.pitch < -89.0f)
					editorCamera.pitch = -89.0f;

				// Calculate Direction vector from yaw and pitch of camera
				editorCamera.direction.x = cos(glm::radians(editorCamera.yaw)) * cos(glm::radians(editorCamera.pitch));
				editorCamera.direction.y = sin(glm::radians(editorCamera.pitch));
				editorCamera.direction.z = sin(glm::radians(editorCamera.yaw)) * cos(glm::radians(editorCamera.pitch));
				editorCamera.direction.Normalise();
			}

			// Calculate Right, Up and LookAt vectors
			editorCamera.right = editorCamera.up.Cross(editorCamera.direction).Normalised();
			editorCamera.lookat = editorCamera.position + editorCamera.direction;

			float newAspect = (float)m_offscreenExtent.width / (float)m_offscreenExtent.height;

			// Recalculate camera perspective if fov has changed, store new fov in prevFov
			if (editorCamera.fov != editorCamera.prevFov || editorCamera.aspect != newAspect)
			{
				UpdateCameraPerspective(editorCamera, editorCamera.fov, newAspect, editorCamera.zNear, editorCamera.zFar);
			}

			editorCamera.matrices.view = UpdateCameraView(editorCamera.position, editorCamera.lookat, editorCamera.up);
		}

		//-------------------------------------------------------------------------------------

		void VulkanRenderSystem::DrawFrame()
		{
			// Wait until gpu has finished rendering last frame. Timeout of 1 second
			VK_CHECK(vkWaitForFences(m_device, 1, &GetCurrentFrame().renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(m_device, 1, &GetCurrentFrame().renderFence)); // Reset fence

			// Recreate Swapchain if window size changes
			if (m_framebufferResized)
			{
				// Get New Window Size
				int width = 0, height = 0;
				glfwGetFramebufferSize(window, &width, &height);

				while (width == 0 || height == 0)
				{
					glfwGetFramebufferSize(window, &width, &height);
					glfwWaitEvents();
				}

				windowExtent.width = width;
				windowExtent.height = height;

				for (int i = 0; i < m_offscreenData.size(); i++)
				{
					m_swapchainData[i].needsRecreated = true;
				}

				// Copy old swapchain and get new one
				m_oldSwapchain = m_swapchain;
				InitSwapchain();

				m_framebufferResized = false;
			}

			// Recreate Viewport if it size changes
			m_viewportSize = m_uiManager->GetWindowViewport()->GetViewportSize();

			if (m_viewportSize.x != m_offscreenExtent.width ||
				m_viewportSize.y != m_offscreenExtent.height)
			{
				for (int i = 0; i < m_offscreenData.size(); i++)
				{
					m_offscreenData[i].needsRecreated = true;
				}

				// Update Offscreen Extents
				m_offscreenExtent.width = m_viewportSize.x;
				m_offscreenExtent.height = m_viewportSize.y;

				if (m_offscreenExtent.width == 0)
				{
					m_offscreenExtent.width = 1024;
				}

				if (m_offscreenExtent.height == 0)
				{
					m_offscreenExtent.height = 1024;
				}

				// Set Forward Render Extent
				m_forwardRenderer->SetRenderExtent(m_offscreenExtent);

				// Setup Deferred Renderer
				//deferredRenderer.RecreateFramebuffer(m_offscreenExtent);
			}

			// Request image from swapchain
			uint32_t swapchainImageIndex;
			VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 0, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

			// Now that we are sure commands are finished executing, reset command buffers
			VK_CHECK(vkResetCommandPool(m_device, GetCurrentFrame().commandPool, 0));

			// Update swapchain data for this frame (if needed)
			UpdatSwapchainData(swapchainImageIndex);

			// Update offscreen for this frame (if needed)
			UpdateOffscreenData(swapchainImageIndex);

			// Copy Debug Vertices to Vertex Buffer
			/*if (GetCurrentFrame().debugVertices.size() > 0)
			{
				CopyVerticesToBuffer(GetCurrentFrame().debugVertices, GetCurrentFrame().debugVertexBuffer);
				CopyIndicesToBuffer(GetCurrentFrame().debugIndices, GetCurrentFrame().debugIndexBuffer);
			}*/

			// Prepare Scene Data for Rendering
			PrepareScene();
			PrepareLights();

			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;

			bool shadowCasterNeedsInitialized = false;

			std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
			ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
			for (const auto& entity : shadowcasterLightEntities)
			{
				shadowCasterNeedsInitialized |= entity->GetComponentFlag<ShadowCasterComponent, FlagDirty>();
			}

			if (!shadowcasterLightEntities.empty() && !shadowCasterNeedsInitialized)
			{
				// Render Shadowmaps
				VkCommandBuffer cmdShadows = RecordShadowCommandBuffers(swapchainImageIndex);
				
				submit.pWaitDstStageMask = nullptr;
				submit.waitSemaphoreCount = 0;
				submit.pWaitSemaphores = nullptr;
				submit.signalSemaphoreCount = 1;
				submit.pSignalSemaphores = &GetCurrentFrame().shadowmapSemaphore;
				submit.commandBufferCount = 1;
				submit.pCommandBuffers = &cmdShadows;

				// Submit Shadowmaps command buffer
				VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE));
			}

			// Forward Render
			VkSemaphore& forwardSemaphore = m_forwardRenderer->DrawScene(frameNumber % FRAME_OVERLAP, swapchainImageIndex);

			// Deferred Render
			/*deferredRenderer.SetGeometryDescriptorSets(&GetCurrentFrame().cameraViewProjDescriptor, 
			&GetCurrentFrame().objectInstanceDescriptor, &GetCurrentFrame().matTextureDescriptor);

			deferredRenderer.SetShadingDescriptorSets(&GetCurrentFrame().cameraShadingDescriptor, 
			&GetCurrentFrame().lightDataDescriptor, &GetCurrentFrame().shadowmapDescriptor);*/

			/*VkSemaphore& deferredSemaphore = deferredRenderer.DrawScene(frameNumber % FRAME_OVERLAP,
						&m_sceneRenderData, graphicsQueue, offscreenFramebuffers[swapchainImageIndex], GetCurrentFrame().shadowmapSemaphore);*/

			// Record Command Buffers
			//VkCommandBuffer cmdMain = RecordMainCommandBuffers(swapchainImageIndex);

			// Pass Offscreen Framebuffer to Viewport Window and Render Viewport
			if (m_offscreenData[swapchainImageIndex].needsRecreated == false)
				m_uiManager->GetWindowViewport()->Draw(m_offscreenData[swapchainImageIndex].viewportTexture);
			else
				m_uiManager->GetWindowViewport()->DrawWithoutImage();

			// Draw ImGui
			ImGui::Render();

 			VkCommandBuffer cmdGui = RecordGUICommandBuffer(swapchainImageIndex);

			std::vector<VkCommandBuffer> submitCommandBuffers = { cmdGui };

			std::vector<VkSemaphore> waitSemaphores = 
			{
				GetCurrentFrame().presentSemaphore,
				forwardSemaphore
			};

			// Prepare the submission into graphics queue
			// we will signal the _renderSemaphore, to signal that rendering has finished

			std::vector<VkPipelineStageFlags> waitStages =
			{
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};

			submit.pWaitDstStageMask = waitStages.data();

			submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
			submit.pWaitSemaphores = waitSemaphores.data();

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

			submit.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
			submit.pCommandBuffers = submitCommandBuffers.data();

			// Submit command buffers to queue and execute
			// GetCurrentFrame().renderFence will now block until graphics command finish executing
			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

			// This will present the image we just rendered to the visible window
			// we want to wait on renderSemaphore for that
			// as its necessary that drawing commands have finished before the image is displayed to the user
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;

			presentInfo.pSwapchains = &m_swapchain;
			presentInfo.swapchainCount = 1;

			presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
			presentInfo.waitSemaphoreCount = 1;

			presentInfo.pImageIndices = &swapchainImageIndex;

			VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

			GetCurrentFrame().debugVertices.clear();
			GetCurrentFrame().debugIndices.clear();
			GetCurrentFrame().debugIndirectCommands.clear();

			// Number of Frames that have completed rendering
			frameNumber++;
		}

		void VulkanRenderSystem::PrepareScene()
		{
			// Map Camera Data to Uniform Buffer
			GPUCameraData cameraData;
			glm::mat4 projMat = editorCamera.matrices.perspective;

			projMat[1][1] *= -1;
			cameraData.viewProj = projMat * editorCamera.matrices.view;

			void* data;

			// Map camera view/proj data to uniform buffer
			vmaMapMemory(m_allocator, GetCurrentFrame().cameraViewProjBuffer.allocation, &data);
			memcpy(data, &cameraData, sizeof(GPUCameraData));
			vmaUnmapMemory(m_allocator, GetCurrentFrame().cameraViewProjBuffer.allocation);

			// Map Mesh Matrices data to GPU 
			MapObjectData();

			// Map indirect commands to buffer
			vmaMapMemory(m_allocator, GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.allocation, &data);

			VkDrawIndexedIndirectCommand* indirectData = (VkDrawIndexedIndirectCommand*)data;

			uint32_t meshIndex = 0;

			// For each mesh data struct, Build Indirect Draw Command
			for (const auto& [fst, snd] : m_sceneRenderData.meshRenderDataMap)
			{
				const auto& meshData = snd;

				indirectData[meshIndex].indexCount = meshData.indexCount;
				indirectData[meshIndex].instanceCount = meshData.entities.size();
				indirectData[meshIndex].firstIndex = meshData.indexOffset;
				indirectData[meshIndex].vertexOffset = meshData.vertexOffset;
				indirectData[meshIndex].firstInstance = meshIndex;

				meshIndex++;
			}

			vmaUnmapMemory(m_allocator, GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.allocation);

			GetCurrentFrame().drawBatch.count = meshIndex;

			deferredRenderer.SetDrawIndirectCommandsBuffer(&GetCurrentFrame().drawBatch);
		}

		void VulkanRenderSystem::UpdateMergedVertexBuffer(const uint32_t newSize)
		{
			// If total vertices exceeds currently allocated buffer size, allocate new buffer
			if (newSize > m_sceneRenderData.vertexBufferSize)
			{
				AllocatedBuffer oldVertexBuffer = m_sceneRenderData.mergedVertexBuffer;
				const uint32_t oldSize = m_sceneRenderData.vertexBufferSize;

				// Double buffer size until it is greater than total vertices
				while (m_sceneRenderData.vertexBufferSize <= newSize)
				{
					m_sceneRenderData.vertexBufferSize *= 2;
				}

				// Create New Buffer
				m_sceneRenderData.mergedVertexBuffer = CreateBuffer(m_sceneRenderData.vertexBufferSize * sizeof(Vertex_PNTV_32),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				// Copy data to new buffer
				ImmediateSubmit([=](VkCommandBuffer cmd)
				{
					// Copy from staging vertex buffer to scene vertex buffer
					VkBufferCopy vertexCopy;
					vertexCopy.dstOffset = 0;
					vertexCopy.size = oldSize * sizeof(Vertex_PNTV_32);
					vertexCopy.srcOffset = 0;

					vkCmdCopyBuffer(cmd, oldVertexBuffer.buffer, m_sceneRenderData.mergedVertexBuffer.buffer, 1, &vertexCopy);
				});

				// Free Old Buffer
				vmaDestroyBuffer(m_allocator, oldVertexBuffer.buffer, oldVertexBuffer.allocation);
			}
		}

		void VulkanRenderSystem::UpdateMergedIndexBuffer(const uint32_t newSize)
		{
			// Same as above for Indices
			if (newSize > m_sceneRenderData.indexBufferSize)
			{
				AllocatedBuffer oldIndexBuffer = m_sceneRenderData.mergedIndexBuffer;
				const uint32_t oldSize = m_sceneRenderData.indexBufferSize;

				// Double buffer size until it is greater than total indices
				while (m_sceneRenderData.indexBufferSize <= newSize)
				{
					m_sceneRenderData.indexBufferSize *= 2;
				}

				// Create New Buffer
				m_sceneRenderData.mergedIndexBuffer = CreateBuffer(m_sceneRenderData.indexBufferSize * sizeof(uint32_t),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				// Copy data to new buffer
				ImmediateSubmit([=](VkCommandBuffer cmd)
				{
					// Copy from staging vertex buffer to scene vertex buffer
					VkBufferCopy indexCopy;
					indexCopy.dstOffset = 0;
					indexCopy.size = oldSize * sizeof(uint32_t);
					indexCopy.srcOffset = 0;

					vkCmdCopyBuffer(cmd, oldIndexBuffer.buffer, m_sceneRenderData.mergedIndexBuffer.buffer, 1, &indexCopy);
				});

				// Free Old Buffer
				vmaDestroyBuffer(m_allocator, oldIndexBuffer.buffer, oldIndexBuffer.allocation);
			}
		}

		inline void VulkanRenderSystem::AddMeshRenderDataToScene(MeshRenderData& meshRenderData, const MeshBufferData& meshBufferData)
		{
			// Resize merged Vertex/Index buffers if new size exceeds old size
			UpdateMergedVertexBuffer(m_sceneRenderData.vertexOffset + meshRenderData.vertexCount);
			UpdateMergedIndexBuffer(m_sceneRenderData.indexOffset + meshRenderData.indexCount);

			meshRenderData.vertexOffset = m_sceneRenderData.vertexOffset;
			meshRenderData.indexOffset = m_sceneRenderData.indexOffset;

			// Init Mesh Staging Buffers
			AllocatedBuffer vertexStagingBuffer = InitVertexBuffer(meshBufferData.vertexData, meshRenderData.vertexCount * meshBufferData.vertexSize);
			AllocatedBuffer indexStagingBuffer = InitIndexBuffer(meshBufferData.indexData, meshRenderData.indexCount * meshBufferData.indexSize);

			// Copy Vertex/Index data to merged buffers
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				// Copy from staging vertex buffer to scene vertex buffer
				VkBufferCopy vertexCopy;
				vertexCopy.dstOffset = m_sceneRenderData.vertexOffset * meshBufferData.vertexSize;
				vertexCopy.size = meshRenderData.vertexCount * meshBufferData.vertexSize;
				vertexCopy.srcOffset = 0;

				vkCmdCopyBuffer(cmd, vertexStagingBuffer.buffer, m_sceneRenderData.mergedVertexBuffer.buffer, 1, &vertexCopy);

				// Copy from staging index buffer to scene index buffer
				VkBufferCopy indexCopy;
				indexCopy.dstOffset = m_sceneRenderData.indexOffset * meshBufferData.indexSize;
				indexCopy.size = meshRenderData.indexCount * meshBufferData.indexSize;
				indexCopy.srcOffset = 0;

				vkCmdCopyBuffer(cmd, indexStagingBuffer.buffer, m_sceneRenderData.mergedIndexBuffer.buffer, 1, &indexCopy);
			});

			// Update scene render data with new info
			m_sceneRenderData.vertexOffset += meshRenderData.vertexCount;
			m_sceneRenderData.indexOffset += meshRenderData.indexCount;

			// Destroy Staging Buffers
			vmaDestroyBuffer(m_allocator, vertexStagingBuffer.buffer, vertexStagingBuffer.allocation);
			vmaDestroyBuffer(m_allocator, indexStagingBuffer.buffer, indexStagingBuffer.allocation);
		}

		void VulkanRenderSystem::PrepareLights()
		{
			// Map shaing data to uniform buffer
			GPUShadingData shadingData;
			shadingData.viewPos = static_cast<glm::vec3>(editorCamera.position);
			shadingData.displayDebugTarget = 0;

			void* data;
			vmaMapMemory(m_allocator, GetCurrentFrame().cameraShadingBuffer.allocation, &data);
			memcpy(data, &shadingData, sizeof(GPUShadingData));
			vmaUnmapMemory(m_allocator, GetCurrentFrame().cameraShadingBuffer.allocation);

			// Map all light data to storage buffer
			void* lightData;
			void* pointLightData;
			void* dirLightData;
			void* spotLightData;

			vmaMapMemory(m_allocator, GetCurrentFrame().lightBuffer.allocation, &lightData);
			vmaMapMemory(m_allocator, GetCurrentFrame().pointLightBuffer.allocation, &pointLightData);
			vmaMapMemory(m_allocator, GetCurrentFrame().dirLightBuffer.allocation, &dirLightData);
			vmaMapMemory(m_allocator, GetCurrentFrame().spotLightBuffer.allocation, &spotLightData);

			GPULightData* lightSSBO = (GPULightData*)lightData;
			GPUPointLightData* pointLightSSBO = (GPUPointLightData*)pointLightData;
			GPUDirLightData* dirLightSSBO = (GPUDirLightData*)dirLightData;
			GPUSpotLightData* spotLightSSBO = (GPUSpotLightData*)spotLightData;

			int l = 0;
			int p = 0;
			int d = 0;
			int s = 0;
			int shadowIndex = 0;

			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
			);

			// For each light map its data to the appropriate storage buffer, incrementing light counter by 1 for each
			std::vector<std::shared_ptr<ECS::Entity>> pointLightEntities;
			ECS::GetEntities<TransformComponent, PointLightComponent>(m_world, pointLightEntities);
			for (const auto& entity : pointLightEntities)
			{
				auto& pointLight = entity->GetComponent<PointLightComponent>();
				const auto& transform = entity->GetComponent<TransformComponent>();

				int tempShadowIndex = -1;

				// TO DO - Add support for point light shadows
				/*if (entity->HasComponent<ShadowCasterComponent>())
				{
					auto& shadowcaster = entity->GetComponent<ShadowCasterComponent>();
				
					tempShadowIndex = shadowIndex;
					shadowIndex++;
				}*/

				lightSSBO[l].ambientColor = static_cast<glm::vec3>(pointLight.ambientColor);
				lightSSBO[l].diffuseColor = static_cast<glm::vec3>(pointLight.diffuseColor);
				lightSSBO[l].specularStrength = pointLight.specularStrength;
				lightSSBO[l].shininess = pointLight.shininess;
				lightSSBO[l].shadowmapIndex = tempShadowIndex;

				pointLightSSBO[p].position = static_cast<glm::vec3>(transform.position);
				pointLightSSBO[p].constant = pointLight.constantAttenuation;
				pointLightSSBO[p].linear = pointLight.linearAttenuation;
				pointLightSSBO[p].quadratic = pointLight.quadraticAttenuation;
				pointLightSSBO[p].dataIndex = l;

				p++;
				l++;
			}

			std::vector<std::shared_ptr<ECS::Entity>> dirLightEntities;
			ECS::GetEntities<TransformComponent, DirectionalLightComponent>(m_world, dirLightEntities);
			for (const auto& entity : dirLightEntities)
			{
				auto& dirLight = entity->GetComponent<DirectionalLightComponent>();

				int tempShadowIndex = -1;

				// TO DO - Add support for directional light shadows
				/*if (entity->HasComponent<ShadowCasterComponent>())
				{
					auto& shadowcaster = entity->GetComponent<ShadowCasterComponent>();
					
					tempShadowIndex = shadowIndex;
					shadowIndex++;
				}*/

				lightSSBO[l].ambientColor = static_cast<glm::vec3>(dirLight.ambientColor);
				lightSSBO[l].diffuseColor = static_cast<glm::vec3>(dirLight.diffuseColor);
				lightSSBO[l].specularStrength = dirLight.specularStrength;
				lightSSBO[l].shininess = dirLight.shininess;
				lightSSBO[l].shadowmapIndex = shadowIndex;

				dirLightSSBO[d].direction = static_cast<glm::vec3>(dirLight.direction);
				dirLightSSBO[d].dataIndex = l;

				d++;
				l++;
			}

			std::vector<std::shared_ptr<ECS::Entity>> spotLightEntities;
			ECS::GetEntities<TransformComponent, SpotLightComponent>(m_world, spotLightEntities);
			for (const auto& entity : spotLightEntities)
			{
				auto& spotLight = entity->GetComponent<SpotLightComponent>();
				const auto& transform = entity->GetComponent<TransformComponent>();

				int tempShadowIndex = -1;

				if (entity->HasComponent<ShadowCasterComponent>())
				{
					auto& shadowcaster = entity->GetComponent<ShadowCasterComponent>();

					const float aspectRatio = static_cast<float>(shadowcaster.shadowmapWidth) / static_cast<float>(shadowcaster.shadowmapHeight);
					shadowcaster.lightSpaceView = CalculateLightSpaceView(aspectRatio, spotLight.outerCutoffAngle, transform.position, spotLight.direction);
					lightSSBO[l].lightSpaceMatrix = biasMatrix * shadowcaster.lightSpaceView;

					tempShadowIndex = shadowIndex;
					shadowIndex++;
				}

				lightSSBO[l].ambientColor = static_cast<glm::vec3>(spotLight.ambientColor);
				lightSSBO[l].diffuseColor = static_cast<glm::vec3>(spotLight.diffuseColor);
				lightSSBO[l].specularStrength = spotLight.specularStrength;
				lightSSBO[l].shininess = spotLight.shininess;
				lightSSBO[l].shadowmapIndex = tempShadowIndex;

				spotLightSSBO[p].position = static_cast<glm::vec3>(transform.position);
				spotLightSSBO[s].direction = static_cast<glm::vec3>(spotLight.direction);
				spotLightSSBO[s].innerCutoff = glm::cos(glm::radians(spotLight.innerCutoffAngle));
				spotLightSSBO[s].outerCutoff = glm::cos(glm::radians(spotLight.outerCutoffAngle));
				spotLightSSBO[s].constant = spotLight.constantAttenuation;
				spotLightSSBO[s].linear = spotLight.linearAttenuation;
				spotLightSSBO[s].quadratic = spotLight.quadraticAttenuation;
				spotLightSSBO[s].dataIndex = l;

				s++;
				l++;
			}

			vmaUnmapMemory(m_allocator, GetCurrentFrame().lightBuffer.allocation);
			vmaUnmapMemory(m_allocator, GetCurrentFrame().pointLightBuffer.allocation);
			vmaUnmapMemory(m_allocator, GetCurrentFrame().dirLightBuffer.allocation);
			vmaUnmapMemory(m_allocator, GetCurrentFrame().spotLightBuffer.allocation);

			GPULightStatsData lightStatsData;
			lightStatsData.numPLights = p;
			lightStatsData.numDLights = d;
			lightStatsData.numSLights = s;

			// Map Light stats data to uniform buffer
			vmaMapMemory(m_allocator, GetCurrentFrame().lightStatsBuffer.allocation, &data);
			memcpy(data, &lightStatsData, sizeof(GPULightStatsData));
			vmaUnmapMemory(m_allocator, GetCurrentFrame().lightStatsBuffer.allocation);
		}

		glm::mat4 VulkanRenderSystem::CalculateLightSpaceView(const float& aspectRatio, const float& outerRadius, const Vector3f& position, const Vector3f& direction) const
		{
			// Near/Far Plane to render depth within
			const float near_plane = 1.0f;
			const float far_plane = 50.0f;

			// Calculate Light Space Projection Matrix
			glm::mat4 lightProj = glm::perspective(
				glm::radians(outerRadius * 2.0f),
				aspectRatio,
				near_plane, far_plane);

			lightProj[1][1] *= -1;

			// Calculate Light Space View Matrix
			glm::mat4 lightView = glm::lookAt(
				glm::vec3(position),
				glm::vec3(position + direction),
				glm::vec3(0.0f, 1.0f, 0.0f));

			return lightProj * lightView;
		}

		VkCommandBuffer VulkanRenderSystem::RecordMainCommandBuffers(uint32_t index)
		{
			// Name buffer cmd for shorter writing
			VkCommandBuffer cmd = GetCurrentFrame().mainCommandBuffer;

			// Begin command buffer recording
			// Let Vulkan know we are only using the buffer once
			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;

			cmdBeginInfo.pInheritanceInfo = nullptr;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Set Clear Color for Framebuffer
			VkClearValue clearValue;
			clearValue.color = { {0.4f, 0.70f, 1.0f, 1.0f} }; // Sky Blue

			// Set Clear Depth Color for Framebuffer
			VkClearValue depthClear;
			depthClear.depthStencil.depth = 1.0f;

			// Start Main Renderpass
			//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
			VkRenderPassBeginInfo rpInfo = {};
			rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpInfo.pNext = nullptr;

			rpInfo.renderPass = m_renderPass;
			rpInfo.renderArea.offset.x = 0;
			rpInfo.renderArea.offset.y = 0;
			rpInfo.renderArea.extent = m_offscreenExtent;
			//rpInfo.framebuffer = offscreenFramebuffers[index];

			// Connect clear values
			VkClearValue clearValues[] = { clearValue, depthClear };

			rpInfo.clearValueCount = 2;
			rpInfo.pClearValues = &clearValues[0];

			// Begin Main Render Pass
			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Draw all Debug Lines/Boxes
			DrawDebugObjects(cmd, index);

			// Finalize Render Pass
			vkCmdEndRenderPass(cmd);

			// Finalize the command buffer (we can no longer add commands, so it can be executed)
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		VkCommandBuffer VulkanRenderSystem::RecordGUICommandBuffer(uint32_t index)
		{
			// Name buffer cmd for shorter writing
			VkCommandBuffer cmd = GetCurrentFrame().guiCommandBuffer;

			// Begin command buffer recording
			// Let Vulkan know we are only using the buffer once
			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;

			cmdBeginInfo.pInheritanceInfo = nullptr;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			// Begin Command buffer
			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Set Clear Color for Framebuffer
			VkClearValue clearValue;
			clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Black

			// Start Main Renderpass
			//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
			VkRenderPassBeginInfo rpInfo = {};
			rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpInfo.pNext = nullptr;

			rpInfo.renderPass = renderPassGUI;
			rpInfo.renderArea.offset.x = 0;
			rpInfo.renderArea.offset.y = 0;
			rpInfo.renderArea.extent = windowExtent;
			rpInfo.framebuffer = m_swapchainData[index].framebuffer;
			rpInfo.clearValueCount = 1;
			rpInfo.pClearValues = &clearValue;

			// Begin Render Pass
			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Record Imgui Draw Data and draw functions into command buffer
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

			vkCmdEndRenderPass(cmd);

			// Finialise Command Buffer
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		VkCommandBuffer VulkanRenderSystem::RecordShadowCommandBuffers(uint32_t index)
		{
			// Name buffer cmd for shorter writing
			VkCommandBuffer cmd = GetCurrentFrame().shadowCommandBuffer;

			// Begin command buffer recording
			// Let Vulkan know we are only using the buffer once
			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;

			cmdBeginInfo.pInheritanceInfo = nullptr;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			// Begin Command buffer
			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Shadow Renderpass
			VkRenderPassBeginInfo shadowRPInfo = {};
			shadowRPInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			shadowRPInfo.pNext = nullptr;
			shadowRPInfo.renderPass = renderPassShadows;
			shadowRPInfo.renderArea.offset.x = 0;
			shadowRPInfo.renderArea.offset.y = 0;

			// Set Clear Depth Color for Framebuffer
			VkClearValue depthClear;
			depthClear.depthStencil.depth = 1.0f;

			shadowRPInfo.clearValueCount = 1;
			shadowRPInfo.pClearValues = &depthClear;

			// Bind Pipeline
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

			// For Each Shadowcasting Light Source
			std::vector<std::shared_ptr<ECS::Entity>> shadowcasterLightEntities;
			ECS::GetEntities<TransformComponent, ShadowCasterComponent>(m_world, shadowcasterLightEntities);
			for (const auto& entity : shadowcasterLightEntities)
			{
				const auto& shadowcaster = entity->GetComponent<ShadowCasterComponent>();

				VkExtent2D shadowmapExtent = { shadowcaster.shadowmapWidth, shadowcaster.shadowmapHeight };

				shadowRPInfo.framebuffer = m_frames[frameNumber % FRAME_OVERLAP].shadowmapFramebuffers[entity->ID()];
				shadowRPInfo.renderArea.extent = shadowmapExtent;

				// Set Pipeline Viewport
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(shadowmapExtent.width);
				viewport.height = static_cast<float>(shadowmapExtent.height);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				vkCmdSetViewport(cmd, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset = { 0, 0 };
				scissor.extent = shadowmapExtent;

				vkCmdSetScissor(cmd, 0, 1, &scissor);

				// Record Shadow Render Pass
				RecordShadowRenderPass(cmd, shadowRPInfo, shadowcaster.lightSpaceView);
			}

			// Finalize Command Buffer
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		void VulkanRenderSystem::RecordShadowRenderPass(VkCommandBuffer cmd, const VkRenderPassBeginInfo& renderPassInfo, const glm::mat4& lightSpaceView)
		{
			// Begin Shadow Render Pass
			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			GPULightSpaceData lightSpaceData;
			lightSpaceData.lightSpaceMatrix = lightSpaceView;

			void* data;
			vmaMapMemory(m_allocator, GetCurrentFrame().lightSpaceBuffer.allocation, &data);
			memcpy(data, &lightSpaceData, sizeof(GPULightSpaceData));
			vmaUnmapMemory(m_allocator, GetCurrentFrame().lightSpaceBuffer.allocation);

			// Bind Light Space DataDescriptors
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				shadowPipelineLayout, 0, 1, &GetCurrentFrame().lightSpaceDescriptor, 0, nullptr);

			// Bind Object Data Descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				shadowPipelineLayout, 1, 1, &GetCurrentFrame().objectInstanceDescriptor, 0, nullptr);

			// Bind Vertex/Index Buffers
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &m_sceneRenderData.mergedVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmd, m_sceneRenderData.mergedIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexedIndirect(cmd, GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.buffer,
				0, GetCurrentFrame().drawBatch.count, sizeof(VkDrawIndexedIndirectCommand));

			// Finalize Render Pass
			vkCmdEndRenderPass(cmd);
		}

		void VulkanRenderSystem::DrawDebugObjects(VkCommandBuffer cmd, uint32_t index)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline);

			// Set Pipeline Viewport
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)m_offscreenExtent.width;
			viewport.height = (float)m_offscreenExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = m_offscreenExtent;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			// Bind Camera View/Proj Descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				debugPipelineLayout, 0, 1, &GetCurrentFrame().cameraViewProjDescriptor, 0, nullptr);

			// Bind Vertex Buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &GetCurrentFrame().debugVertexBuffer.buffer, offsets);

			// Map Indirect Commands to buffer
			void* data;
			vmaMapMemory(m_allocator, GetCurrentFrame().debugIndirectCommandsBuffer.allocation, &data);
			memcpy(data, GetCurrentFrame().debugIndirectCommands.data(), sizeof(VkDrawIndexedIndirectCommand) * GetCurrentFrame().debugIndirectCommands.size());
			vmaUnmapMemory(m_allocator, GetCurrentFrame().debugIndirectCommandsBuffer.allocation);

			// Bind Index Buffer
			vkCmdBindIndexBuffer(cmd, GetCurrentFrame().debugIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			
			uint32_t draw_count = GetCurrentFrame().debugIndirectCommands.size();
			uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);

			// Draw Debug Lines/Boxes Using Draw Indirect
			vkCmdDrawIndexedIndirect(cmd, GetCurrentFrame().debugIndirectCommandsBuffer.buffer, 0, draw_count, draw_stride);
		}

		void VulkanRenderSystem::MapObjectData()
		{
			// Map all instance/object data to storage buffers
			void* objectData;
			vmaMapMemory(m_allocator, GetCurrentFrame().objectBuffer.allocation, &objectData);

			void* instanceData;
			vmaMapMemory(m_allocator, GetCurrentFrame().instanceBuffer.allocation, &instanceData);

			GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
			GPUInstanceData* instanceSSBO = (GPUInstanceData*)instanceData;

			int o = 0;
			int i = 0;

			// For each instance, map object offset
			for (const auto& [fst, snd] : m_sceneRenderData.meshRenderDataMap)
			{
				const auto& meshData = snd;

				// Map Instance Data
				instanceSSBO[i].objectOffset = o;

				// For each instance entity, calculate its object data and map to storage buffer
				for (auto entity : meshData.entities)
				{
					const auto& transform = m_world->GetComponent<TransformComponent>(entity);

#ifdef PFN_USE_DOUBLE_PRECISION
					Vector3d position = Vector3d(0.0);
					Vector3d interpolatedPosition = Vector3d(0.0);
#else
					Vector3f position = Vector3f(0.0f);
					Vector3f interpolatedPosition = Vector3f(0.0f);
#endif

					if (m_world->HasComponent<Physics::VelocityComponent>(entity))
					{
						const auto& velocity = m_world->GetComponent<Physics::VelocityComponent>(entity);
						interpolatedPosition = transform.position + (velocity.linear * m_engine->GetDeltaTime());

						position = Maths::Lerp(transform.position, interpolatedPosition,
							m_engine->GetAccumulatedTime() / m_engine->GetTimeStep());
					}
					else
					{
						position = transform.position;
					}

					objectSSBO[o].model = BuildMeshTransform(position, transform.rotation, transform.scale);
					objectSSBO[o].inv_model = glm::inverse(objectSSBO[o].model);

					o++;
				}

				i++;
			}

			vmaUnmapMemory(m_allocator, GetCurrentFrame().objectBuffer.allocation);
			vmaUnmapMemory(m_allocator, GetCurrentFrame().instanceBuffer.allocation);
		}

		glm::mat4 VulkanRenderSystem::BuildMeshTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale) const
		{
			// Set Translation
			glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), (glm::vec3)position);

			// Set Rotation
			model_transform = glm::rotate(model_transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, -1.0f));

			// Set Scale
			model_transform = glm::scale(model_transform, (glm::vec3)scale);

			return model_transform;
		}

		//-------------------------------------------------------------------------------------

		void VulkanRenderSystem::Cleanup()
		{
			if (isInitialized)
			{
				// Make sure GPU has stopped working
				vkWaitForFences(m_device, 1, &GetCurrentFrame().renderFence, true, 1000000000);

				// Cleanup Forward/Deferred Renderer
				m_forwardRenderer->Cleanup();
				m_forwardRenderer = nullptr;

				deferredRenderer.Cleanup();

				// Flush all queues, destroying all created resources

				vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
				for (int i = 0; i < m_swapchainData.size(); i++)
				{
					DestroySwapchainData(i);
				}

				for (int i = 0; i < m_offscreenData.size(); i++)
				{
					DestroyOffscreenData(i);
				}

				mainDeletionQueue.flush();

				Stop();

				CleanupBuffers();

				// Cleanup Allocator/Cache
				descriptorAllocator->Cleanup();
				descriptorLayoutCache->Cleanup();

				vkDestroyDevice(m_device, nullptr);
				vkDestroySurfaceKHR(instance, surface, nullptr);
				vkDestroyInstance(instance, nullptr);
			}
		}

		void VulkanRenderSystem::CleanupBuffers()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				vmaDestroyBuffer(m_allocator, m_frames[i].cameraViewProjBuffer.buffer, m_frames[i].cameraViewProjBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].objectBuffer.buffer, m_frames[i].objectBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].instanceBuffer.buffer, m_frames[i].instanceBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].lightSpaceBuffer.buffer, m_frames[i].lightSpaceBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].debugVertexBuffer.buffer, m_frames[i].debugVertexBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].debugIndexBuffer.buffer, m_frames[i].debugIndexBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].debugIndirectCommandsBuffer.buffer, m_frames[i].debugIndirectCommandsBuffer.allocation);

				vmaDestroyBuffer(m_allocator, m_frames[i].drawBatch.drawIndirectCommandsBuffer.buffer, m_frames[i].drawBatch.drawIndirectCommandsBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].cameraShadingBuffer.buffer, m_frames[i].cameraShadingBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].lightBuffer.buffer, m_frames[i].lightBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].pointLightBuffer.buffer, m_frames[i].pointLightBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].dirLightBuffer.buffer, m_frames[i].dirLightBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].spotLightBuffer.buffer, m_frames[i].spotLightBuffer.allocation);
				vmaDestroyBuffer(m_allocator, m_frames[i].lightStatsBuffer.buffer, m_frames[i].lightStatsBuffer.allocation);
			}

			vmaDestroyBuffer(m_allocator, m_sceneRenderData.mergedVertexBuffer.buffer, m_sceneRenderData.mergedVertexBuffer.allocation);
			vmaDestroyBuffer(m_allocator, m_sceneRenderData.mergedIndexBuffer.buffer, m_sceneRenderData.mergedIndexBuffer.allocation);
		}

		//-------------------------------------------------------------------------------------

		AllocatedBuffer VulkanRenderSystem::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags)
		{
			// Set what the usage of this buffer is
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;
			bufferInfo.size = allocSize;
			bufferInfo.usage = usage;

			// Let VMA library whether this buffer needs to be used by CPU, GPU, etc...
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = memoryUsage;
			allocInfo.requiredFlags = requiredFlags;

			AllocatedBuffer newBuffer;

			// Allocate Buffer
			VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));

			return newBuffer;
		}

		void VulkanRenderSystem::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
		{
			// Allocate Default command buffer that will be used for instant commands
			VkCommandBufferAllocateInfo cmdAllocInfo = VKInit::CommandBufferAllocateInfo(uploadContext.commandPool, 1);

			VkCommandBuffer cmd;
			VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &cmd));

			// Begin Command buffer recording. The command buffer will be used only once, so let vulkan know that
			VkCommandBufferBeginInfo cmdBeginInfo = VKInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Execute Function
			function(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));

			VkSubmitInfo submit = VKInit::SubmitInfo(&cmd);

			// Submit Command Buffer to queue and execute
			// uploadFence will now block until the graphics command finish execution
			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, uploadContext.uploadFence));

			VK_CHECK(vkWaitForFences(m_device, 1, &uploadContext.uploadFence, true, 9999999999));
			VK_CHECK(vkResetFences(m_device, 1, &uploadContext.uploadFence));

			// Clear command pool and free command buffer
			VK_CHECK(vkResetCommandPool(m_device, uploadContext.commandPool, 0));
		}

		//-------------------------------------------------------------------------------------

		void VulkanRenderSystem::DrawDebugLine(Debug::Line line)
		{
			// Create debug line vertices to current frames vertices vector
			Vertex_PC_32 startVertex, endVertex;
			startVertex.pos = static_cast<glm::vec3>(line.start);
			startVertex.color = static_cast<glm::vec3>(line.color);

			endVertex.pos = static_cast<glm::vec3>(line.end);
			endVertex.color = static_cast<glm::vec3>(line.color);

			// Create Indexed Indirect Draw Command for Vertices
			VkDrawIndexedIndirectCommand command = {};
			command.indexCount = 2;
			command.instanceCount = 1;
			command.firstIndex = GetCurrentFrame().debugIndices.size();
			command.vertexOffset = 0;
			command.firstInstance = GetCurrentFrame().debugIndirectCommands.size();

			uint32_t firstIndex = GetCurrentFrame().debugVertices.size();
			
			// Add vertices to vector
			GetCurrentFrame().debugVertices.push_back(startVertex);
			GetCurrentFrame().debugVertices.push_back(endVertex);

			// Add indices to vector
			GetCurrentFrame().debugIndices.push_back(firstIndex);
			GetCurrentFrame().debugIndices.push_back(firstIndex + 1);

			// Add draw indirect command to current frame commands vector
			GetCurrentFrame().debugIndirectCommands.push_back(command);
		}

		void VulkanRenderSystem::DrawDebugBox(Debug::Box box)
		{
			const int numVertices = 4;
			const int numIndices = 8;

			const int firstVertex = GetCurrentFrame().debugVertices.size();
			const int firstIndex = GetCurrentFrame().debugIndices.size();

			Vertex_PC_32 vert = {};
			vert.color = static_cast<glm::vec3>(box.color);

			// Add Vertices to vector
			for (int i = 0; i < numVertices; i++)
			{
				//Vector2f point = origin + Maths::Rot
			}

			// Add Indices to vector
			for (int i = 0; i < numIndices; i++)
			{
				
			}
		}

		void VulkanRenderSystem::DrawDebugCube(Debug::Cube cube)
		{
			const int numVertices = 8;
			const int numIndices = 24;

			int firstVertex = GetCurrentFrame().debugVertices.size();
			int firstIndex = GetCurrentFrame().debugIndices.size();

			Vertex_PC_32 vert = {};
			vert.color = static_cast<glm::vec3>(cube.color);

			// Add Vertices to vector
			for (int i = 0; i < numVertices; i++)
			{
				vert.pos = static_cast<glm::vec3>(cube.origin + (cube.halfSize * cubePositions[i]));
				GetCurrentFrame().debugVertices.push_back(vert);
			}

			// Add Indices to vector
			for (int i = 0; i < numIndices; i++)
			{
				GetCurrentFrame().debugIndices.push_back(firstVertex + cubeIndices[i]);
			}

			// Create Indexed Indirect Draw Command for Vertices
			VkDrawIndexedIndirectCommand command = {};
			command.indexCount = numIndices;
			command.instanceCount = 1;
			command.firstIndex = firstIndex;
			command.vertexOffset = 0;
			command.firstInstance = GetCurrentFrame().debugIndirectCommands.size();

			GetCurrentFrame().debugIndirectCommands.push_back(command);
		}
	}
}