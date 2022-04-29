#include "VulkanEngine.h"

#include <Rendering/VKInitializers.h>
#include <Rendering/VKDescriptors.h>
#include <Rendering/VKTexture.h>
#include <Rendering/VKDebug.h>

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"

#include "Types/ComponentFlags.h"

#define VMA_IMPLEMENTATION
#include <Rendering/vk_mem_alloc.h>

//#include "renderdoc_app.h"
#include <Rendering/DebugDraw.h>

#include <iostream>
#include <string>
#include <algorithm>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace Puffin
{
	namespace Rendering
	{
		GLFWwindow* VulkanEngine::Init(GLFWwindow* windowIn, std::shared_ptr<UI::UIManager> inUIManager, std::shared_ptr<Input::InputManager> inInputManager)
		{
			m_uiManager = inUIManager;
			m_inputManager = inInputManager;

			windowExtent.width = WIDTH;
			windowExtent.height = HEIGHT;

			// Initialize Offscreen Variables with Default Values
			offscreenExtent.width = 1024;
			offscreenExtent.height = 1024;
			offscreenFormat = VK_FORMAT_R8G8B8A8_SRGB;

			// Initialize Camera Variables
			camera.position = glm::vec3(0.0f, 0.0f, 10.0f);
			camera.aspect = (float)offscreenExtent.width / (float)offscreenExtent.height;
			InitCamera(camera);

			viewportSize = ImVec2(0.0f, 0.0f);
			offscreenInitialized = false;
			shadowmapDescriptorNeedsUpdated = false;

			// Initialize Shadowmap Resolution/Format
			shadowExtent.width = 1024;
			shadowExtent.height = 1024;
			shadowFormat = VK_FORMAT_D16_UNORM;

			window = windowIn;

			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
			glfwMaximizeWindow(window);

			// Load Core Vulkan Structures
			InitVulkan();

			// Create Swapchain
			InitSwapchain();

			// Create Offscreen Variables
			InitOffscreen();

			// Initialize Command Pool and buffer
			InitCommands();

			// Initialise Default Renderpass
			InitDefaultRenderpass();

			// Initialize GUI Renderpass
			InitGUIRenderpass();

			// Initialize Shadow Renderpass
			InitShadowRenderPass();

			// Initialise Swapchain Framebuffers
			InitFramebuffers();

			// Initialize Offscreen Framebuffers
			InitOffscreenFramebuffers();

			// Initialise Semaphores and Fences
			InitSyncStructures();

			InitBuffers();
			InitSceneBuffers();

			// Initialize Descriptor Sets
			InitDescriptors();

			// Initialize Pipelines
			InitPipelines();

			InitShadowPipeline();

			InitDebugPipeline();

			InitTextureSampler();

			InitDepthSampler();

			// Initialize All Scene Objects
			Start();

			// Setup Deferred Renderer
			SetupDeferredRenderer();

			// Pass Camera to UI
			m_uiManager->GetWindowSettings()->SetCamera(&camera);

			// Subscribe to events
			m_inputEvents = std::make_shared<RingBuffer<Input::InputEvent>>();
			m_drawLineEvents = std::make_shared<RingBuffer<Debug::Line>>();
			m_drawBoxEvents = std::make_shared<RingBuffer<Debug::Box>>();

			m_world->RegisterEvent<Debug::Line>();
			m_world->RegisterEvent<Debug::Box>();

			m_world->SubscribeToEvent<Input::InputEvent>(m_inputEvents);
			m_world->SubscribeToEvent<Debug::Line>(m_drawLineEvents);
			m_world->SubscribeToEvent<Debug::Box>(m_drawBoxEvents);

			// Initialize ImGui
			InitImGui();

			InitImGuiTextureIDs();

			m_uiManager->GetWindowViewport()->SetTextureSampler(textureSampler);

			isInitialized = true;

			return window;
		}

		void VulkanEngine::Start()
		{
			InitScene();
			InitDeferredDescriptors();
			InitShadowmapDescriptors();
		}

		void VulkanEngine::InitVulkan()
		{
			vkb::InstanceBuilder builder;

			// Init Vulkan Instance
			auto inst_ret = builder.set_app_name("Puffin Engine")
				.request_validation_layers(enableValidationLayers)
				.require_api_version(1, 2, 0)
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

			VkPhysicalDeviceDescriptorIndexingFeatures supportedDescriptorFeatures = {};
			supportedDescriptorFeatures.runtimeDescriptorArray = VK_TRUE;

			// Select GPU with VK Bootstrap
			// We want a gpu which can write to glfw surface and supports vulkan 1.2
			vkb::PhysicalDeviceSelector selector{ vkb_inst };
			vkb::PhysicalDevice vkbPhysicalDevice = selector
				.set_minimum_version(1, 2)
				.set_surface(surface)
				.set_required_features(supportedFeatures)
				.add_desired_extension("VK_EXT_debug_utils")
				.select()
				.value();

			// Setup VKDebug Function Pointers
			VKDebug::Setup(instance);

			// Create Final Vulkan Device
			vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
			vkb::Device vkbDevice = deviceBuilder
				.add_pNext(&supportedDescriptorFeatures)
				.build()
				.value();

			// Get VKDevice handle used in rest of vulkan application
			device = vkbDevice.device;
			physicalDevice = vkbPhysicalDevice.physical_device;

			// Use vkbootstrap to get a graphics queue
			graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
			graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

			// Initialize Memory Allocator
			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = physicalDevice;
			allocatorInfo.device = device;
			allocatorInfo.instance = instance;

			// Create Allocator
			vmaCreateAllocator(&allocatorInfo, &allocator);
		}

		void VulkanEngine::InitSwapchain()
		{
			vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, surface };

			vkb::Swapchain vkbSwapchain = swapchainBuilder
				.use_default_format_selection()
				// VK_PRESENT_MODE_FIFO_KHR for double buffering, VK_PRESENT_MODE_MAILBOX_KHR for Triple buffering
				.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
				.set_desired_extent(windowExtent.width, windowExtent.height)
				.build()
				.value();

			// Store Swapchain and related images/views
			swapchain = vkbSwapchain.swapchain;
			swapchainAttachments.clear();

			for (int i = 0; i < vkbSwapchain.image_count; i++)
			{
				AllocatedImage swapchainAttachment;
				swapchainAttachment.image = vkbSwapchain.get_images().value()[i];

				// Set Debug Name for RenderDoc
				std::string string = "Swapchain Image " + std::to_string(i);

				VKDebug::SetObjectName(device,
					(uint64_t)swapchainAttachment.image,
					VK_OBJECT_TYPE_IMAGE,
					string.c_str());

				swapchainAttachment.imageView = vkbSwapchain.get_image_views().value()[i];
				swapchainAttachments.push_back(swapchainAttachment);
			}

			swapchainImageFormat = vkbSwapchain.image_format;

			swapchainDeletionQueue.push_function([=]()
			{
				vkDestroySwapchainKHR(device, swapchain, nullptr);
			});
		}

		void VulkanEngine::InitOffscreen()
		{
			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();

			// Create Images/Views for Offscreen Rendering
			offscreenAttachments = std::vector<AllocatedImage>(swapchain_imagecount);

			VkExtent3D imageExtent =
			{
				offscreenExtent.width,
				offscreenExtent.height,
				1
			};

			// Init Image/Allocation Info
			VkImageCreateInfo imageInfo = VKInit::ImageCreateInfo(offscreenFormat, 
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
			VmaAllocationCreateInfo imageAllocInfo = {};
			imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			for (int i = 0; i < swapchain_imagecount; i++)
			{
				// Create Image
				vmaCreateImage(allocator, &imageInfo, &imageAllocInfo,
					&offscreenAttachments[i].image, &offscreenAttachments[i].allocation, nullptr);

				// Set Debug Name for RenderDoc
				std::string string = "Offscreen Framebuffer " + std::to_string(i);

				VKDebug::SetObjectName(device,
					(uint64_t)offscreenAttachments[i].image,
					VK_OBJECT_TYPE_IMAGE,
					string.c_str());

				// Create Image View
				VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(offscreenFormat,
					offscreenAttachments[i].image, VK_IMAGE_ASPECT_COLOR_BIT);

				VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &offscreenAttachments[i].imageView));

				// Add to deletion queues
				offscreenDeletionQueue.push_function([=]()
				{
					vkDestroyImageView(device, offscreenAttachments[i].imageView, nullptr);
					vmaDestroyImage(allocator, offscreenAttachments[i].image, offscreenAttachments[i].allocation);
				});
			}

			// Create Depth Image

			// Depth image size will match window
			VkExtent3D depthImageExtent =
			{
				offscreenExtent.width,
				offscreenExtent.height,
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

			// Allocate and create image
			vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthAttachment.image, &depthAttachment.allocation, nullptr);

			// Set Debug Name for RenderDoc
			VKDebug::SetObjectName(device,
				(uint64_t)depthAttachment.image,
				VK_OBJECT_TYPE_IMAGE,
				"Offscreen Depth Image");

			// Build Image View for depth image to use in rendering
			VkImageViewCreateInfo depthImageViewInfo = VKInit::ImageViewCreateInfo(depthFormat, depthAttachment.image, VK_IMAGE_ASPECT_DEPTH_BIT);

			VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthAttachment.imageView));

			// Add to deletion queues
			offscreenDeletionQueue.push_function([=]()
			{
				vkDestroyImageView(device, depthAttachment.imageView, nullptr);
				vmaDestroyImage(allocator, depthAttachment.image, depthAttachment.allocation);
			});
		}

		void VulkanEngine::InitCommands()
		{
			// Create Command Pool for commands submitted to graphics queue
			// we also want the pool to allow for resetting individual command buffers
			VkCommandPoolCreateInfo commandPoolInfo = VKInit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			// Allocate Command Pool/Buffer for each frame data struct
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Allocate Command Pools
				VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

				// Allocate Default GUI and Shadow Command Buffer that we will use for scene rendering
				VkCommandBufferAllocateInfo allocInfo = VKInit::CommandBufferAllocateInfo(frames[i].commandPool, 1); 
				VkCommandBufferAllocateInfo allocInfoGui = VKInit::CommandBufferAllocateInfo(frames[i].commandPool, 1);
				VkCommandBufferAllocateInfo allocInfoShadow = VKInit::CommandBufferAllocateInfo(frames[i].commandPool, 1);

				// Allocate buffers
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].mainCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfoGui, &frames[i].guiCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfoShadow, &frames[i].shadowCommandBuffer));

				// Push destruction of both command pools/buffers to deletion queue
				mainDeletionQueue.push_function([=]() 
				{
					vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
				});
			}

			// Create Upload Command Pool
			VkCommandPoolCreateInfo uploadCommandPoolInfo = VKInit::CommandPoolCreateInfo(graphicsQueueFamily);

			VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyCommandPool(device, uploadContext.commandPool, nullptr);
			});
		}

		void VulkanEngine::InitDefaultRenderpass()
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

			VK_CHECK(vkCreateRenderPass(device, &render_pass_info, nullptr, &renderPass));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(device, renderPass, nullptr);
			});
		}

		void VulkanEngine::InitGUIRenderpass()
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

			VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPassGUI));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(device, renderPassGUI, nullptr);
			});
		}

		void VulkanEngine::InitShadowRenderPass()
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

			VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPassShadows));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(device, renderPassShadows, nullptr);
			});
		}

		void VulkanEngine::InitFramebuffers()
		{
			// Create Framebuffers for swapchain images.
			// This will connect render pass to images for rendering

			// Create Info for Swapchain Framebuffers
			VkFramebufferCreateInfo fb_gui_info = {};
			fb_gui_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_gui_info.pNext = nullptr;

			fb_gui_info.renderPass = renderPassGUI;
			fb_gui_info.width = windowExtent.width;
			fb_gui_info.height = windowExtent.height;
			fb_gui_info.layers = 1;

			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();
			framebuffers.clear();
			framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				// Attach swapchain image view to Framebuffer
				fb_gui_info.pAttachments = &swapchainAttachments[i].imageView;
				fb_gui_info.attachmentCount = 1;

				VK_CHECK(vkCreateFramebuffer(device, &fb_gui_info, nullptr, &framebuffers[i]));

				// Push all deletion functions to queue
				swapchainDeletionQueue.push_function([=]() {
					vkDestroyFramebuffer(device, framebuffers[i], nullptr);
					vkDestroyImageView(device, swapchainAttachments[i].imageView, nullptr);
				});
			}
		}

		void VulkanEngine::InitOffscreenFramebuffers()
		{
			// Create Info for Offscreen Framebuffers
			VkFramebufferCreateInfo fb_offscreen_info = {};
			fb_offscreen_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_offscreen_info.pNext = nullptr;

			fb_offscreen_info.renderPass = renderPass;
			fb_offscreen_info.width = offscreenExtent.width;
			fb_offscreen_info.height = offscreenExtent.height;
			fb_offscreen_info.layers = 1;

			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();
			offscreenFramebuffers.clear();
			offscreenFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

			// Create Framebuffers for each of the swapchain image views
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				// Attach offscreen and depth image view to Framebuffer
				VkImageView attachments[2];
				attachments[0] = offscreenAttachments[i].imageView;
				attachments[1] = depthAttachment.imageView;

				fb_offscreen_info.pAttachments = attachments;
				fb_offscreen_info.attachmentCount = 2;

				VK_CHECK(vkCreateFramebuffer(device, &fb_offscreen_info, nullptr, &offscreenFramebuffers[i]));

				offscreenDeletionQueue.push_function([=]()
				{
					vkDestroyFramebuffer(device, offscreenFramebuffers[i], nullptr);
				});
			}
		}

		void VulkanEngine::InitSyncStructures()
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
				VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));

				//enqueue the destruction of the fence
				mainDeletionQueue.push_function([=]() {
					vkDestroyFence(device, frames[i].renderFence, nullptr);
				});

				VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].presentSemaphore));
				VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));

				//enqueue the destruction of semaphores
				mainDeletionQueue.push_function([=]() {
					vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
					vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
				});
			}

			// Create Upload Fence
			VkFenceCreateInfo uploadCreateFenceInfo = VKInit::FenceCreateInfo();

			VK_CHECK(vkCreateFence(device, &uploadCreateFenceInfo, nullptr, &uploadContext.uploadFence));

			// enqueue destruction of upload fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(device, uploadContext.uploadFence, nullptr);
			});
		}

		void VulkanEngine::InitBuffers()
		{
			// Create Buffers for each Frame's Data
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Camera View/Proj Buffer
				frames[i].cameraViewProjBuffer = CreateBuffer(sizeof(GPUCameraData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Create Object Storage Buffers
				
				frames[i].objectBuffer = CreateBuffer(sizeof(GPUObjectData) * MAX_OBJECTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Create Camera/Light Buffers
				frames[i].cameraBuffer = CreateBuffer(sizeof(ViewData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Create Light Space Buffer for Shadow Vertex Stage
				frames[i].lightSpaceBuffer = CreateBuffer(sizeof(GPULightSpaceData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].lightSpaceMultiBuffer = CreateBuffer(sizeof(GPULightSpaceData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].lightSpaceIndexBuffer = CreateBuffer(sizeof(GPULightIndexData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Debug Buffers
				frames[i].debugVertexBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * MAX_VERTICES_PER_COMMAND * sizeof(Vertex),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				frames[i].debugIndexBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * MAX_INDICES_PER_COMMAND * sizeof(uint32_t),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

				frames[i].debugIndirectCommandsBuffer = CreateBuffer(MAX_DEBUG_COMMANDS * sizeof(VkDrawIndexedIndirectCommand),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].debugVertices.reserve(MAX_DEBUG_COMMANDS * MAX_VERTICES_PER_COMMAND);
				frames[i].debugIndices.reserve(MAX_DEBUG_COMMANDS * MAX_INDICES_PER_COMMAND);
			}
		}

		void VulkanEngine::InitSceneBuffers()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Model Data
				frames[i].drawBatch.drawIndirectCommandsBuffer = CreateBuffer(MAX_OBJECTS * sizeof(VkDrawIndexedIndirectCommand),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Camera/Debug Buffer
				frames[i].uboBuffer = CreateBuffer(sizeof(ShadingUBO),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Light Buffers
				frames[i].pointLightBuffer = CreateBuffer(sizeof(GPUPointLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].dirLightBuffer = CreateBuffer(sizeof(GPUDirLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].spotLightBuffer = CreateBuffer(sizeof(GPUSpotLightData) * MAX_LIGHTS_PER_TYPE,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Light Stats Buffer
				frames[i].lightStatsBuffer = CreateBuffer(sizeof(GPULightStatsData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			}

			// Merged Vertex/Index Buffers
			sceneData.mergedVertexBuffer = CreateBuffer(CURRENT_VERTEX_BUFFER_SIZE * sizeof(Vertex),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

			sceneData.mergedIndexBuffer = CreateBuffer(CURRENT_INDEX_BUFFER_SIZE * sizeof(uint32_t),
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		}

		void VulkanEngine::InitDescriptors()
		{
			// Initialize Descriptor Abstractions
			descriptorAllocator = new VKUtil::DescriptorAllocator{};
			descriptorAllocator->Init(device);

			descriptorLayoutCache = new VKUtil::DescriptorLayoutCache{};
			descriptorLayoutCache->Init(device);

			// Initialize Texture Descriptor Layout
			VkDescriptorSetLayoutBinding textureBinding = VKInit::DescriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

			VkDescriptorSetLayoutCreateInfo textureLayoutInfo = {};
			textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			textureLayoutInfo.flags = 0;
			textureLayoutInfo.pNext = nullptr;
			textureLayoutInfo.bindingCount = 1;
			textureLayoutInfo.pBindings = &textureBinding;

			singleTextureSetLayout = descriptorLayoutCache->CreateDescriptorSetLayout(&textureLayoutInfo);

			// Create Shadowmap Descriptor Layout
			VkDescriptorSetLayoutBinding shadowmapBinding = VKInit::DescriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, MAX_LIGHTS_PER_TYPE * 3);

			VkDescriptorSetLayoutCreateInfo shadowmapLayoutInfo = {};
			shadowmapLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			shadowmapLayoutInfo.flags = 0;
			shadowmapLayoutInfo.pNext = nullptr;
			shadowmapLayoutInfo.bindingCount = 1;
			shadowmapLayoutInfo.pBindings = &shadowmapBinding;

			shadowMapSetLayout = descriptorLayoutCache->CreateDescriptorSetLayout(&shadowmapLayoutInfo);

			// Create Uniform/Storage Buffer for each frame
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Descriptor Sets for Vertex Shader Stage

				// Allocate Camera View/Proj Descriptor Set
				VkDescriptorBufferInfo cameraBufferInfo;
				cameraBufferInfo.buffer = frames[i].cameraViewProjBuffer.buffer;
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(GPUCameraData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].cameraViewProjDescriptor, cameraViewProjSetLayout);

				// Allocate Object Descriptor Set
				VkDescriptorBufferInfo objectBufferInfo;
				objectBufferInfo.buffer = frames[i].objectBuffer.buffer;
				objectBufferInfo.offset = 0;
				objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].objectDescriptor, objectSetLayout);

				// Create Descriptor Sets for Fragment Shader Stage

				// Allocate Camera Descriptor Set
				VkDescriptorBufferInfo cameraInfo;
				cameraInfo.buffer = frames[i].cameraBuffer.buffer;
				cameraInfo.offset = 0;
				cameraInfo.range = sizeof(ViewData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &cameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frames[i].cameraDescriptor, cameraSetLayout);

				// Allocate Lights Descriptor Set
				VkDescriptorBufferInfo pointLightInfo;
				pointLightInfo.buffer = frames[i].pointLightBuffer.buffer;
				pointLightInfo.offset = 0;
				pointLightInfo.range = sizeof(GPUPointLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo directionalLightInfo;
				directionalLightInfo.buffer = frames[i].dirLightBuffer.buffer;
				directionalLightInfo.offset = 0;
				directionalLightInfo.range = sizeof(GPUDirLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo spotLightInfo;
				spotLightInfo.buffer = frames[i].spotLightBuffer.buffer;
				spotLightInfo.offset = 0;
				spotLightInfo.range = sizeof(GPUSpotLightData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo lightStatInfo;
				lightStatInfo.buffer = frames[i].lightStatsBuffer.buffer;
				lightStatInfo.offset = 0;
				lightStatInfo.range = sizeof(GPULightStatsData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &pointLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(1, &directionalLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(2, &spotLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(3, &lightStatInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frames[i].lightDescriptor, lightSetLayout);

				VkDescriptorBufferInfo lightSpaceInfo;
				lightSpaceInfo.buffer = frames[i].lightSpaceBuffer.buffer;
				lightSpaceInfo.offset = 0;
				lightSpaceInfo.range = sizeof(GPULightSpaceData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &lightSpaceInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].lightSpaceDescriptor, lightSpaceSetLayout);

				// Initialize Light Space Descriptor Layout for Shadow/Shader Vertex Stage
				VkDescriptorBufferInfo lightSpaceMultiInfo;
				lightSpaceMultiInfo.buffer = frames[i].lightSpaceMultiBuffer.buffer;
				lightSpaceMultiInfo.offset = 0;
				lightSpaceMultiInfo.range = sizeof(GPULightSpaceData) * MAX_LIGHTS_PER_TYPE;

				VkDescriptorBufferInfo lightSpaceIndexInfo;
				lightSpaceIndexInfo.buffer = frames[i].lightSpaceIndexBuffer.buffer;
				lightSpaceIndexInfo.offset = 0;
				lightSpaceIndexInfo.range = sizeof(GPULightIndexData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &lightSpaceMultiInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.BindBuffer(1, &lightSpaceIndexInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].lightSpaceMultiDescriptor, lightSpaceMultiSetLayout);
			}
		}

		// Init Descriptors that will be passed to the Deferred Renderer
		void VulkanEngine::InitDeferredDescriptors()
		{
			// Grab Image Views from Initialized Meshes
			std::vector<VkDescriptorImageInfo> albedoImageInfo;
			std::vector<VkDescriptorImageInfo> normalImageInfo;

			VkDescriptorImageInfo textureImageInfo;
			textureImageInfo.sampler = depthSampler;
			textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

				//Albedo Textures
				textureImageInfo.imageView = mesh.texture.imageView;
				albedoImageInfo.push_back(textureImageInfo);

				// Normal Maps	
				textureImageInfo.imageView = mesh.texture.imageView;
				normalImageInfo.push_back(textureImageInfo);
			}

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Descriptor Bindings

				// Camera ViewProj
				VkDescriptorBufferInfo cameraBufferInfo;
				cameraBufferInfo.buffer = frames[i].cameraViewProjBuffer.buffer;
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(GPUCameraData);

				// Object SSBO
				VkDescriptorBufferInfo objectBufferInfo;
				objectBufferInfo.buffer = frames[i].objectBuffer.buffer;
				objectBufferInfo.offset = 0;
				objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

				if (albedoImageInfo.size() > 0 && normalImageInfo.size() > 0)
				{

					// Build Descriptor Set
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.BindBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
						.BindBuffer(1, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
						.BindImages(2, static_cast<uint32_t>(albedoImageInfo.size()),
							albedoImageInfo.data(),
							VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.BindImages(3, static_cast<uint32_t>(normalImageInfo.size()),
							normalImageInfo.data(),
							VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
						.Build(frames[i].geometryDescriptor, geometrySetLayout);
				}
				else
				{
					// Build Descriptor Set
					VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
						.BindBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
						.BindBuffer(1, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
						.Build(frames[i].geometryDescriptor, geometrySetLayout);
				}
			}
		}

		void VulkanEngine::InitPipelines()
		{
			// Read Shader Code from files
			auto vertShaderCode = ReadFile("C:\\Projects\\PuffinProject\\content\\shaders\\default_lighting_vert.spv");
			auto fragShaderCode = ReadFile("C:\\Projects\\PuffinProject\\content\\shaders\\default_lighting_frag.spv");

			// Create Shader Modules from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				cameraViewProjSetLayout,
				objectSetLayout,
				lightSpaceMultiSetLayout,
				cameraSetLayout,
				lightSetLayout,
				shadowMapSetLayout,
				singleTextureSetLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &meshMaterial.pipelineLayout));

			// Create Pipeline Builder object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Define Viewport
			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)offscreenExtent.width;
			pipelineBuilder.viewport.height = (float)offscreenExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			// Define Scissor Extent (Pixels Outside Scissor Rectangle will be discarded)
			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = offscreenExtent;

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			VkPipelineColorBlendAttachmentState colorBlendAttachment = VKInit::ColorBlendAttachmentState();
			pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(1, &colorBlendAttachment);

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			std::vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			// Dynamic Viewport/Scissor Size
			pipelineBuilder.dynamic = VKInit::DynamicStateCreateInfo(dynamicStates);

			// Assign Pipeline Layout to Pipeline
			pipelineBuilder.pipelineLayout = meshMaterial.pipelineLayout;

			// Build Pipeline
			meshMaterial.pipeline = pipelineBuilder.build_pipeline(device, renderPass);
		}

		void VulkanEngine::InitShadowPipeline()
		{
			// Read Shader Code from files
			auto vertShaderCode = ReadFile("C:\\Projects\\PuffinProject\\content\\shaders\\shadowmap_vert.spv");

			// Create Shader Module from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				lightSpaceSetLayout,
				objectSetLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout));

			// Create Pipeline Builder Object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));

			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)shadowExtent.width;
			pipelineBuilder.viewport.height = (float)shadowExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			// Define Scissor Extent
			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = shadowExtent;

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);

			// Multisampling
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			VkPipelineColorBlendAttachmentState colorBlendAttachment = VKInit::ColorBlendAttachmentState();
			pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(1, &colorBlendAttachment);

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Assign Pipeline Layout
			pipelineBuilder.pipelineLayout = shadowPipelineLayout;

			// Build Pipeline
			shadowPipeline = pipelineBuilder.build_pipeline(device, renderPassShadows);
		}

		void VulkanEngine::InitDebugPipeline()
		{
			// Read Shader Code from Files
			auto vertShaderCode = ReadFile("C:\\Projects\\PuffinProject\\content\\shaders\\debug_vert.spv");
			auto fragShaderCode = ReadFile("C:\\Projects\\PuffinProject\\content\\shaders\\debug_frag.spv");

			// Create Shader Modules from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				cameraViewProjSetLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &debugPipelineLayout));

			// Pipeline Builder Object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDesciptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDesciptions);

			// Create Input Assembly Info - Will do line rendering her instead of filled triangle rendering
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

			// Define Viewport
			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)offscreenExtent.width;
			pipelineBuilder.viewport.height = (float)offscreenExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			// Define Scissor Extent (Pixels Outside Scissor Rectangle will be discarded)
			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = offscreenExtent;

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
			debugPipeline = pipelineBuilder.build_pipeline(device, renderPass);
		}

		void VulkanEngine::InitScene()
		{
			// Initialize Lights
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);

				InitLight(light);
			}

			// Initialize Meshes
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);
				InitMesh(mesh);
			}

		}

		void VulkanEngine::InitMesh(MeshComponent& mesh)
		{
			// Load Texture Data
			//IO::LoadImageFromFile(*this, mesh.texture_path, mesh.texture);

			const auto textureAsset = std::static_pointer_cast<Assets::TextureAsset>(Assets::AssetRegistry::Get()->GetAsset(mesh.textureAssetID));

			if (textureAsset && textureAsset->Load())
			{
				IO::InitTextureImage(*this, textureAsset->GetPixels(), textureAsset->GetTextureWidth(), textureAsset->GetTextureHeight(), mesh.texture);

				textureAsset->Unload();
			}

			VKDebug::SetObjectName(device,
				(uint64_t)mesh.texture.image,
				VK_OBJECT_TYPE_IMAGE,
				"Mesh Texture");

			VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_UNORM, mesh.texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &mesh.texture.imageView));

			const auto staticMeshAsset = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(mesh.meshAssetID));

			// Load Mesh Data
			if (staticMeshAsset && staticMeshAsset->Load())
			{
				mesh.vertexCount = static_cast<uint32_t>(staticMeshAsset->GetVertices().size());
				mesh.indexCount = static_cast<uint32_t>(staticMeshAsset->GetIndices().size());

				// Init Mesh Buffers
				mesh.vertexBuffer = InitVertexBuffer(staticMeshAsset->GetVertices());
				mesh.indexBuffer = InitIndexBuffer(staticMeshAsset->GetIndices());

				staticMeshAsset->Unload();
			}

			mesh.material = meshMaterial;

			// Write descriptor set so it points to mesh texture
			VkDescriptorImageInfo imageBufferInfo;
			imageBufferInfo.sampler = textureSampler;
			imageBufferInfo.imageView = mesh.texture.imageView;
			imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindImage(0, &imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build(mesh.material.textureSet, singleTextureSetLayout);
		}

		void VulkanEngine::InitLight(LightComponent& light)
		{
			// Create Depth Image for Light
			VkExtent3D imageExtent =
			{
				shadowExtent.width,
				shadowExtent.height,
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
			framebufferInfo.width = shadowExtent.width;
			framebufferInfo.height = shadowExtent.height;
			framebufferInfo.layers = 1;

			light.depthAttachments = std::vector<AllocatedImage>(FRAME_OVERLAP);
			light.depthFramebuffers = std::vector<VkFramebuffer>(FRAME_OVERLAP);

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Create Image/Allocation
				vmaCreateImage(allocator, &imageInfo, &imageAllocInfo,
					&light.depthAttachments[i].image, &light.depthAttachments[i].allocation, nullptr);

				std::string string = "Light Depth Image " + std::to_string(i);

				VKDebug::SetObjectName(device,
					(uint64_t)light.depthAttachments[i].image,
					VK_OBJECT_TYPE_IMAGE,
					string.c_str());

				// Create Image View
				VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(shadowFormat,
					light.depthAttachments[i].image, VK_IMAGE_ASPECT_DEPTH_BIT);

				VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &light.depthAttachments[i].imageView));

				// Create Framebuffer
				framebufferInfo.pAttachments = &light.depthAttachments[i].imageView;
				framebufferInfo.attachmentCount = 1;

				VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &light.depthFramebuffers[i]));
			}
		}

		void VulkanEngine::InitCamera(CameraComponent& camera)
		{
			// Calculate Perspective Projection
			camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);

			// Calculate Right and Up Vectors
			camera.right = glm::normalize(glm::cross(camera.up, camera.direction));
			camera.up = glm::cross(camera.direction, camera.right);
			camera.lookat = camera.position + camera.direction;

			// Calculate Camera View Matrix
			camera.matrices.view = glm::lookAt(camera.position, camera.lookat, camera.up);
		}

		AllocatedBuffer VulkanEngine::InitVertexBuffer(const std::vector<Vertex>& vertices)
		{
			// Copy Loaded Mesh data into mesh vertex buffer
			const size_t bufferSize = vertices.size() * sizeof(Vertex);

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, vertices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
			AllocatedBuffer vertexBuffer = CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT |VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			// Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

			return vertexBuffer;
		}

		AllocatedBuffer VulkanEngine::InitIndexBuffer(const std::vector<uint32_t>& indices)
		{
			// Copy Loaded Index data into mesh index buffer
			const size_t bufferSize = indices.size() * sizeof(uint32_t);

			// Allocated Staging Buffer - Map Indices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map Index data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, indices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Allocate Index Buffer - Transfer Indices into GPU Memory
			AllocatedBuffer indexBuffer = CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, indexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);

			return indexBuffer;
		}

		void VulkanEngine::CopyVerticesToBuffer(const std::vector<Vertex>& vertices, AllocatedBuffer vertexBuffer, uint32_t copyOffset)
		{
			// Get Size of data to be transfered to vertex buffer
			const size_t bufferSize = vertices.size() * sizeof(Vertex);

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, vertices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = copyOffset * sizeof(Vertex);
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanEngine::CopyIndicesToBuffer(const std::vector<uint32_t>& indices, AllocatedBuffer indexBuffer, uint32_t copyOffset)
		{
			// Copy Loaded Index data into mesh index buffer
			const size_t bufferSize = indices.size() * sizeof(uint32_t);

			// Allocated Staging Buffer - Map Indices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map Index data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, indices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = copyOffset;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, indexBuffer.buffer, 1, &copy);
			});

			// Cleanup Staging Buffer
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanEngine::CleanupMesh(MeshComponent& mesh)
		{
			// Cleanup Texture
			vmaDestroyImage(allocator, mesh.texture.image, mesh.texture.allocation);

			// Cleanup Vertex/Index Buffers
			vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
			vmaDestroyBuffer(allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
		}

		void VulkanEngine::CleanupLight(LightComponent& light)
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Destroy Image/View
				vkDestroyImageView(device, light.depthAttachments[i].imageView, nullptr);
				vmaDestroyImage(allocator, light.depthAttachments[i].image, light.depthAttachments[i].allocation);

				// Destroy Framebuffer
				vkDestroyFramebuffer(device, light.depthFramebuffers[i], nullptr);
			}
		}

		void VulkanEngine::InitTextureSampler()
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

			VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
		}

		void VulkanEngine::InitDepthSampler()
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

			VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &depthSampler));
		}

		void VulkanEngine::SetupDeferredRenderer()
		{
			std::vector<VkCommandPool> commandPools;
			std::vector<AllocatedBuffer> uboBuffers;
			std::vector<AllocatedBuffer> lightBuffers;

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				commandPools.push_back(frames[i].commandPool);

				uboBuffers.push_back(frames[i].uboBuffer);

				lightBuffers.push_back(frames[i].pointLightBuffer);
				lightBuffers.push_back(frames[i].dirLightBuffer);
				lightBuffers.push_back(frames[i].spotLightBuffer);
				lightBuffers.push_back(frames[i].lightStatsBuffer);
			}

			deferredRenderer.Setup(physicalDevice,
				device,
				allocator,
				descriptorAllocator,
				descriptorLayoutCache,
				commandPools,
				FRAME_OVERLAP, offscreenExtent);

			deferredRenderer.SetupGeometry(geometrySetLayout);
			deferredRenderer.SetupShading(uboBuffers, MAX_LIGHTS_PER_TYPE, lightBuffers, renderPass);
		}

		void VulkanEngine::InitShadowmapDescriptors()
		{
			if (entityMap["Light"].empty())
			{
				return;
			}

			// Write Shadowmap Descriptor Sets
			VkDescriptorImageInfo shadowmapBufferInfo;
			shadowmapBufferInfo.sampler = depthSampler;
			shadowmapBufferInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			// Initialise Shadowmap Descriptor
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				std::vector<VkDescriptorImageInfo> imageInfos;

				for (ECS::Entity entity : entityMap["Light"])
				{
					LightComponent& light = m_world->GetComponent<LightComponent>(entity);

					shadowmapBufferInfo.imageView = light.depthAttachments[i].imageView;
					imageInfos.push_back(shadowmapBufferInfo);
				}

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindImages(0, static_cast<uint32_t>(imageInfos.size()), imageInfos.data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frames[i].shadowmapDescriptor, shadowMapSetLayout);
			}
		}

		void VulkanEngine::InitImGui()
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
			VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool));


			// Initialize imgui library
			// this initializes the core structures of imgui
			//ImGui::CreateContext();

			// Initialize ImGui for GLFW
			ImGui_ImplGlfw_InitForVulkan(window, true);

			//this initializes imgui for Vulkan
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = instance;
			init_info.PhysicalDevice = physicalDevice;
			init_info.Device = device;
			init_info.Queue = graphicsQueue;
			init_info.DescriptorPool = imguiPool;
			init_info.MinImageCount = 3;
			init_info.ImageCount = 3;

			ImGui_ImplVulkan_Init(&init_info, renderPass);

			//execute a gpu command to upload imgui font textures
			ImmediateSubmit([&](VkCommandBuffer cmd) 
			{
				ImGui_ImplVulkan_CreateFontsTexture(cmd);
			});

			//clear font textures from cpu data
			ImGui_ImplVulkan_DestroyFontUploadObjects();

			//add the destroy the imgui created structures
			mainDeletionQueue.push_function([=]() {

				vkDestroyDescriptorPool(device, imguiPool, nullptr);
				ImGui_ImplVulkan_Shutdown();
			});
		}

		void VulkanEngine::InitImGuiTextureIDs()
		{
			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();

			viewportTextureIDs.clear();
			viewportTextureIDs = std::vector<ImTextureID>(swapchain_imagecount);

			// Create Texture ID's for rendering Viewport to ImGui Window
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				viewportTextureIDs[i] = ImGui_ImplVulkan_AddTexture(textureSampler, offscreenAttachments[i].imageView,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::RecreateSwapchain()
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

			// Cleanup Swapchain Variables
			swapchainDeletionQueue.flush();

			InitSwapchain();
			InitFramebuffers();

			framebufferResized = false;
		}

		void VulkanEngine::RecreateOffscreen()
		{
			// Delete all Offscreen Variables in deletion queue
			offscreenDeletionQueue.flush();

			// Update Offscreen Extents
			offscreenExtent.width = viewportSize.x;
			offscreenExtent.height = viewportSize.y;

			// Setup Deferred Renderer
			deferredRenderer.Cleanup();

			std::vector<VkCommandPool> commandPools;
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				commandPools.push_back(frames[i].commandPool);
			}
			
			SetupDeferredRenderer();

			// Initialize Offscreen Variables and Scene
			InitOffscreen();
			InitOffscreenFramebuffers();
			InitImGuiTextureIDs();

			offscreenInitialized = true;
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Update()
		{
			ProcessEvents();

			// Initialize/Cleanup marked components
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

				// Initialize
				if (m_world->GetComponentFlag<MeshComponent, FlagDirty>(entity))
				{
					InitMesh(mesh);

					m_world->SetComponentFlag<MeshComponent, FlagDirty>(entity, false);

					sceneData.bFlagSceneChanged = true;
				}

				// Cleanup
				if (m_world->GetComponentFlag<MeshComponent, FlagDeleted>(entity) || m_world->GetEntityFlag<FlagDeleted>(entity))
				{
					CleanupMesh(mesh);
					m_world->RemoveComponent<MeshComponent>(entity);
					
					sceneData.bFlagSceneChanged = true;
				}
			}

			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);

				// Initialize

				if (m_world->GetComponentFlag<LightComponent, FlagDirty>(entity))
				{
					InitLight(light);

					m_world->SetComponentFlag<LightComponent, FlagDirty>(entity, false);

					shadowmapDescriptorNeedsUpdated = true;
					sceneData.bFlagSceneChanged = true;
				}

				// Cleanup
				if (m_world->GetComponentFlag<LightComponent, FlagDeleted>(entity) || m_world->GetEntityFlag<FlagDeleted>(entity))
				{
					CleanupLight(light);
					m_world->RemoveComponent<LightComponent>(entity);
					shadowmapDescriptorNeedsUpdated = true;
					
					sceneData.bFlagSceneChanged = true;
				}
			}

			// Rebuild Shadowmaps if needed
			if (shadowmapDescriptorNeedsUpdated)
			{
				InitShadowmapDescriptors();
				shadowmapDescriptorNeedsUpdated = false;
			}

			m_uiManager->DrawUI(m_deltaTime, m_inputManager);

			UpdateCamera(camera);

			DrawFrame();
		}

		void VulkanEngine::Stop()
		{
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);
				CleanupMesh(mesh);
			}

			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);
				CleanupLight(light);
			}
		}

		void VulkanEngine::ProcessEvents()
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

		void VulkanEngine::UpdateCamera(CameraComponent& camera)
		{
			if (m_inputManager->IsCursorLocked())
			{
				// Camera Movement
				if (moveLeft && !moveRight)
				{
					camera.position += camera.speed * camera.right * (float)m_deltaTime;
				}
				
				if (moveRight && !moveLeft)
				{
					camera.position -= camera.speed * camera.right * (float)m_deltaTime;
				}

				if (moveForward && !moveBackward)
				{
					camera.position += camera.speed * camera.direction * (float)m_deltaTime;
				}
				
				if (moveBackward && !moveForward)
				{
					camera.position -= camera.speed * camera.direction * (float)m_deltaTime;
				}

				if (moveUp && !moveDown)
				{
					camera.position += camera.speed * camera.up * (float)m_deltaTime;
				}
				
				if (moveDown && !moveUp)
				{
					camera.position -= camera.speed * camera.up * (float)m_deltaTime;
				}

				// Mouse Rotation
				camera.yaw += m_inputManager->GetMouseXOffset();
				camera.pitch -= m_inputManager->GetMouseYOffset();

				if (camera.pitch > 89.0f)
					camera.pitch = 89.0f;

				if (camera.pitch < -89.0f)
					camera.pitch = -89.0f;

				// Calculate Direction vector from yaw and pitch of camera
				camera.direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
				camera.direction.y = sin(glm::radians(camera.pitch));
				camera.direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
				camera.direction = glm::normalize(camera.direction);
			}

			// Calculate Right, Up and LookAt vectors
			camera.right = glm::normalize(glm::cross(camera.up, camera.direction));
			camera.lookat = camera.position + camera.direction;

			float newAspect = (float)offscreenExtent.width / (float)offscreenExtent.height;

			// Recalculate camera perspective if fov has changed, store new fov in prevFov
			if (camera.fov != camera.prevFov || camera.aspect != newAspect)
			{
				camera.aspect = newAspect;
				camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
				camera.prevFov = camera.fov;
			}

			camera.matrices.view = glm::lookAt(camera.position, camera.lookat, camera.up);
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::DrawFrame()
		{
			// Wait until gpu has finished rendering last frame. Timeout of 1 second
			VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence)); // Reset fence
			
			// Request image from swapchain
			uint32_t swapchainImageIndex;
			VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 0, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

			// Now that we are sure commands are finished executing, reset command buffers
			VK_CHECK(vkResetCommandPool(device, GetCurrentFrame().commandPool, 0));

			// Pass Offscreen Framebuffer to Viewport Window and Render Viewport
			if (offscreenInitialized)
				m_uiManager->GetWindowViewport()->Draw(viewportTextureIDs[swapchainImageIndex], camera);
			else
				m_uiManager->GetWindowViewport()->DrawWithoutImage();

			viewportSize = m_uiManager->GetWindowViewport()->GetViewportSize();

			// Draw ImGui
			ImGui::Render();

			// Recreate Swapchain if window size changes
			if (framebufferResized)
			{
				RecreateSwapchain();
			}

			// Recreate Viewport if it size changes
			if (!offscreenInitialized || 
				viewportSize.x != offscreenExtent.width ||
				viewportSize.y != offscreenExtent.height)
			{
				RecreateOffscreen();
			}

			// Copy Debug Vertices to Vertex Buffer
			if (GetCurrentFrame().debugVertices.size() > 0)
			{
				CopyVerticesToBuffer(GetCurrentFrame().debugVertices, GetCurrentFrame().debugVertexBuffer);
				CopyIndicesToBuffer(GetCurrentFrame().debugIndices, GetCurrentFrame().debugIndexBuffer);
			}

			// Prepare Scene Data for Rendering
			PrepareScene();
			PrepareLights();

			// Deferred Render
			deferredRenderer.SetGeometryDescriptorSet(&GetCurrentFrame().geometryDescriptor);
			VkSemaphore& deferredSemaphore = deferredRenderer.DrawScene(frameNumber % FRAME_OVERLAP, &sceneData, graphicsQueue, offscreenFramebuffers[swapchainImageIndex]);

			// Record Command Buffers
			//VkCommandBuffer cmdShadows = RecordShadowCommandBuffers(swapchainImageIndex);
			//VkCommandBuffer cmdMain = RecordMainCommandBuffers(swapchainImageIndex);
 			VkCommandBuffer cmdGui = RecordGUICommandBuffer(swapchainImageIndex);

			std::vector<VkCommandBuffer> submitCommandBuffers = { /*cmdShadows, cmdMain,*/ cmdGui };

			std::vector<VkSemaphore> waitSemaphores = 
			{
				GetCurrentFrame().presentSemaphore,
				deferredSemaphore
			};

			// Prepare the submission into graphics queue
			// we will signal the _renderSemaphore, to signal that rendering has finished
			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;

			std::vector<VkPipelineStageFlags> waitStages =
			{
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
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

			presentInfo.pSwapchains = &swapchain;
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

		void VulkanEngine::PrepareScene()
		{
			// Map Camera Data to Uniform Buffer
			GPUCameraData cameraData;
			glm::mat4 projMat = camera.matrices.perspective;
			projMat[1][1] *= -1;
			cameraData.viewProj = projMat * camera.matrices.view;

			void* data;

			// Map camera view/proj data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().cameraViewProjBuffer.allocation, &data);
			memcpy(data, &cameraData, sizeof(GPUCameraData));
			vmaUnmapMemory(allocator, GetCurrentFrame().cameraViewProjBuffer.allocation);

			// Map Mesh Matrices date to GPU 
			MapObjectData();

			// Map indirect commands to buffer
			vmaMapMemory(allocator, GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.allocation, &data);

			VkDrawIndexedIndirectCommand* indirectData = (VkDrawIndexedIndirectCommand*)data;

			uint32_t meshIndex = 0;
			uint32_t totalVertices = 0;
			uint32_t totalIndices = 0;

			// Build Draw Indirect Commands
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

				indirectData[meshIndex].indexCount = mesh.indexCount;
				indirectData[meshIndex].instanceCount = 1;
				indirectData[meshIndex].firstIndex = totalIndices;
				indirectData[meshIndex].vertexOffset = totalVertices;
				indirectData[meshIndex].firstInstance = meshIndex;

				totalVertices += mesh.vertexCount;
				totalIndices += mesh.indexCount;
				meshIndex++;
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.allocation);

			GetCurrentFrame().drawBatch.count = meshIndex;

			deferredRenderer.SetDrawIndirectCommandsBuffer(&GetCurrentFrame().drawBatch);

			// Update Vertex/Index Merged Buffer
			if (sceneData.bFlagSceneChanged == true)
			{
				// Map Mesh Matrices to Storage Buffer
				totalVertices = 0;
				totalIndices = 0;

				// Check how many vertices there are
				for (ECS::Entity entity : entityMap["Mesh"])
				{
					MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

					totalVertices += mesh.vertexCount;
					totalIndices += mesh.indexCount;
				}

				// If total vertices exceeds currently allocted buffer size, allocate new buffer
				if (totalVertices > CURRENT_VERTEX_BUFFER_SIZE)
				{
					// Free Old Buffer
					vmaDestroyBuffer(allocator,
						sceneData.mergedVertexBuffer.buffer,
						sceneData.mergedVertexBuffer.allocation);

					// Double buffer size until it is greater than total vertices
					while (CURRENT_VERTEX_BUFFER_SIZE <= totalVertices)
					{
						CURRENT_VERTEX_BUFFER_SIZE *= 2;
					}

					// Create New Buffer
					sceneData.mergedVertexBuffer = CreateBuffer(CURRENT_VERTEX_BUFFER_SIZE * sizeof(Vertex),
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
				}

				// Same as above for Indices
				if (totalIndices > CURRENT_INDEX_BUFFER_SIZE)
				{
					// Free Old Buffer
					vmaDestroyBuffer(allocator,
						sceneData.mergedIndexBuffer.buffer,
						sceneData.mergedIndexBuffer.allocation);

					// Double buffer size until it is greater than total indices
					while (CURRENT_INDEX_BUFFER_SIZE <= totalIndices)
					{
						CURRENT_INDEX_BUFFER_SIZE *= 2;
					}

					// Create New Buffer
					sceneData.mergedIndexBuffer = CreateBuffer(CURRENT_INDEX_BUFFER_SIZE * sizeof(uint32_t),
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
				}

				// Copy Vertices/Indices to GPU buffer
				ImmediateSubmit([=](VkCommandBuffer cmd)
				{
					uint32_t vertexOffset = 0;
					uint32_t indexOffset = 0;

					// Copy Vertices/Indices for each mesh to Buffers
					for (ECS::Entity entity : entityMap["Mesh"])
					{
						MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

						// Copy directly from mesh vertex buffer to scene vertex buffer
						VkBufferCopy vertexCopy;
						vertexCopy.dstOffset = vertexOffset * sizeof(Vertex);
						vertexCopy.size = mesh.vertexCount * sizeof(Vertex);
						vertexCopy.srcOffset = 0;

						vkCmdCopyBuffer(cmd, mesh.vertexBuffer.buffer, sceneData.mergedVertexBuffer.buffer, 1, &vertexCopy);

						// Copt directly from mesh index buffer to scene index buffer
						VkBufferCopy indexCopy;
						indexCopy.dstOffset = indexOffset * sizeof(uint32_t);
						indexCopy.size = mesh.indexCount * sizeof(uint32_t);
						indexCopy.srcOffset = 0;

						vkCmdCopyBuffer(cmd, mesh.indexBuffer.buffer, sceneData.mergedIndexBuffer.buffer, 1, &indexCopy);

						vertexOffset += mesh.vertexCount;
						indexOffset += mesh.indexCount;
					}
				});

				sceneData.bFlagSceneChanged = false;
			}
		}

		void VulkanEngine::PrepareLights()
		{
			// Map shaing data to uniform buffer
			ShadingUBO uboData;
			uboData.viewPos = camera.position;
			uboData.displayDebugTarget = 0;

			void* data;
			vmaMapMemory(allocator, GetCurrentFrame().uboBuffer.allocation, &data);
			memcpy(data, &uboData, sizeof(ShadingUBO));
			vmaUnmapMemory(allocator, GetCurrentFrame().uboBuffer.allocation);

			// Map all light data to storage buffer
			void* pointLightData;
			void* dirLightData;
			void* spotLightData;

			vmaMapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation, &pointLightData);
			vmaMapMemory(allocator, GetCurrentFrame().dirLightBuffer.allocation, &dirLightData);
			vmaMapMemory(allocator, GetCurrentFrame().spotLightBuffer.allocation, &spotLightData);

			GPUPointLightData* pointLightSSBO = (GPUPointLightData*)pointLightData;
			GPUDirLightData* dirLightSSBO = (GPUDirLightData*)dirLightData;
			GPUSpotLightData* spotLightSSBO = (GPUSpotLightData*)spotLightData;

			int p = 0;
			int d = 0;
			int s = 0;
			int lightIndex = 0;

			// For each light map its data to the appropriate storage buffer, incrementing light counter by 1 for each
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);

				int shadowIndex = -1;

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.bFlagCastShadows && light.type == LightType::SPOT)
				{
					shadowIndex = lightIndex;
					lightIndex++;
				}

				switch (light.type)
				{
				case LightType::POINT:
					pointLightSSBO[p].position = m_world->GetComponent<TransformComponent>(entity).position;
					pointLightSSBO[p].ambientColor = light.ambientColor;
					pointLightSSBO[p].diffuseColor = light.diffuseColor;
					pointLightSSBO[p].constant = light.constantAttenuation;
					pointLightSSBO[p].linear = light.linearAttenuation;
					pointLightSSBO[p].quadratic = light.quadraticAttenuation;
					pointLightSSBO[p].specularStrength = light.specularStrength;
					pointLightSSBO[p].shininess = light.shininess;
					pointLightSSBO[p].shadowmapIndex = shadowIndex;
					p++;
					break;
				case LightType::DIRECTIONAL:
					dirLightSSBO[d].ambientColor = light.ambientColor;
					dirLightSSBO[d].diffuseColor = light.diffuseColor;
					dirLightSSBO[d].direction = light.direction;
					dirLightSSBO[d].specularStrength = light.specularStrength;
					dirLightSSBO[d].shininess = light.shininess;
					dirLightSSBO[d].shadowmapIndex = shadowIndex;
					d++;
					break;
				case LightType::SPOT:
					spotLightSSBO[s].position = m_world->GetComponent<TransformComponent>(entity).position;
					spotLightSSBO[s].direction = light.direction;
					spotLightSSBO[s].ambientColor = light.ambientColor;
					spotLightSSBO[s].diffuseColor = light.diffuseColor;
					spotLightSSBO[s].innerCutoff = glm::cos(glm::radians(light.innerCutoffAngle));
					spotLightSSBO[s].outerCutoff = glm::cos(glm::radians(light.outerCutoffAngle));
					spotLightSSBO[s].constant = light.constantAttenuation;
					spotLightSSBO[s].linear = light.linearAttenuation;
					spotLightSSBO[s].quadratic = light.quadraticAttenuation;
					spotLightSSBO[s].specularStrength = light.specularStrength;
					spotLightSSBO[s].shininess = light.shininess;
					spotLightSSBO[s].shadowmapIndex = shadowIndex;
					s++;
					break;
				}
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().dirLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().spotLightBuffer.allocation);

			GPULightStatsData lightStatsData;
			lightStatsData.numPLights = p;
			lightStatsData.numDLights = d;
			lightStatsData.numSLights = s;

			// Map Light stats data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().lightStatsBuffer.allocation, &data);
			memcpy(data, &lightStatsData, sizeof(GPULightStatsData));
			vmaUnmapMemory(allocator, GetCurrentFrame().lightStatsBuffer.allocation);
		}

		VkCommandBuffer VulkanEngine::RecordMainCommandBuffers(uint32_t index)
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

			rpInfo.renderPass = renderPass;
			rpInfo.renderArea.offset.x = 0;
			rpInfo.renderArea.offset.y = 0;
			rpInfo.renderArea.extent = offscreenExtent;
			rpInfo.framebuffer = offscreenFramebuffers[index];

			// Connect clear values
			VkClearValue clearValues[] = { clearValue, depthClear };

			rpInfo.clearValueCount = 2;
			rpInfo.pClearValues = &clearValues[0];

			// Begin Main Render Pass
			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Draw all Mesh objects
			DrawObjects(cmd, index);

			// Draw all Debug Lines/Boxes
			DrawDebugObjects(cmd, index);

			// Finalize Render Pass
			vkCmdEndRenderPass(cmd);

			// Finalize the command buffer (we can no longer add commands, so it can be executed)
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		VkCommandBuffer VulkanEngine::RecordGUICommandBuffer(uint32_t index)
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
			rpInfo.framebuffer = framebuffers[index];
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

		VkCommandBuffer VulkanEngine::RecordShadowCommandBuffers(uint32_t index)
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
			shadowRPInfo.renderArea.extent = shadowExtent;

			// Set Clear Depth Color for Framebuffer
			VkClearValue depthClear;
			depthClear.depthStencil.depth = 1.0f;

			shadowRPInfo.clearValueCount = 1;
			shadowRPInfo.pClearValues = &depthClear;

			// Bind Pipeline
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

			int lightIndex = 0;

			// For Each Shadowcasting Light Source
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.bFlagCastShadows && light.type == LightType::SPOT)
				{
					TransformComponent& transform = m_world->GetComponent<TransformComponent>(entity);

					shadowRPInfo.framebuffer = light.depthFramebuffers[frameNumber % FRAME_OVERLAP];

					// Begin Shadow Render Pass
					vkCmdBeginRenderPass(cmd, &shadowRPInfo, VK_SUBPASS_CONTENTS_INLINE);

					// Near/Far Plane to render depth within
					float near_plane = 1.0f;
					float far_plane = 50.0f;

					// Calculate Light Space Projection Matrix
					glm::mat4 lightProj = glm::perspective(
						glm::radians(light.outerCutoffAngle * 2.0f),
						(float)shadowExtent.width / (float)shadowExtent.height,
						near_plane, far_plane);

					lightProj[1][1] *= -1;

					glm::mat4 lightView = glm::lookAt(
						glm::vec3(transform.position),
						glm::vec3(transform.position + light.direction),
						glm::vec3(0.0f, 1.0f, 0.0f));

					light.lightSpaceView = lightProj * lightView;

					GPULightSpaceData lightSpaceData;
					lightSpaceData.lightSpaceMatrix = light.lightSpaceView;

					void* data;
					vmaMapMemory(allocator, GetCurrentFrame().lightSpaceBuffer.allocation, &data);
					memcpy(data, &lightSpaceData, sizeof(GPULightSpaceData));
					vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceBuffer.allocation);

					// Bind Light Space DataDescriptors
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						shadowPipelineLayout, 0, 1, &GetCurrentFrame().lightSpaceDescriptor, 0, nullptr);

					// Bind Object Data Descriptor
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						shadowPipelineLayout, 1, 1, &GetCurrentFrame().objectDescriptor, 0, nullptr);

					int meshIndex = 0;

					// Render Depth Map for this Light
					for (ECS::Entity entity : entityMap["Mesh"])
					{
						MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

						if (!m_world->GetComponentFlag<MeshComponent, FlagDirty>(entity) &&
							!m_world->GetComponentFlag<MeshComponent, FlagDeleted>(entity))
						{
							// Bind Vertices, Indices and Descriptor Sets
							VkDeviceSize offsets[] = { 0 };
							vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offsets);
							vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

							// Draw Indexed Vertices
							vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indexCount), 1, 0, 0, meshIndex);

							meshIndex++;
						}
					}

					// Finalize Render Pass
					vkCmdEndRenderPass(cmd);

					lightIndex++;
				}
			}

			// Finialise Command Buffer
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		void VulkanEngine::DrawObjects(VkCommandBuffer cmd, uint32_t index)
		{
			// Map all light data to storage buffer
			void* pointLightData;
			void* dirLightData;
			void* spotLightData;
			void* lightSpaceData;

			vmaMapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation, &pointLightData);
			vmaMapMemory(allocator, GetCurrentFrame().dirLightBuffer.allocation, &dirLightData);
			vmaMapMemory(allocator, GetCurrentFrame().spotLightBuffer.allocation, &spotLightData);
			vmaMapMemory(allocator, GetCurrentFrame().lightSpaceMultiBuffer.allocation, &lightSpaceData);

			GPUPointLightData* pointLightSSBO = (GPUPointLightData*)pointLightData;
			GPUDirLightData* dirLightSSBO = (GPUDirLightData*)dirLightData;
			GPUSpotLightData* spotLightSSBO = (GPUSpotLightData*)spotLightData;
			GPULightSpaceData* lightSpaceSSBO = (GPULightSpaceData*)lightSpaceData;

			int p = 0;
			int d = 0;
			int s = 0;
			int lightIndex = 0;

			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
			);

			// For each light map its data to the appropriate storage buffer, incrementing light counter by 1 for each
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = m_world->GetComponent<LightComponent>(entity);

				int shadowIndex = -1;

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.bFlagCastShadows && light.type == LightType::SPOT)
				{
					lightSpaceSSBO[lightIndex].lightSpaceMatrix = biasMatrix * light.lightSpaceView;

					shadowIndex = lightIndex;
					lightIndex++;
				}

				switch (light.type)
				{
				case LightType::POINT:
					pointLightSSBO[p].position = m_world->GetComponent<TransformComponent>(entity).position;
					pointLightSSBO[p].ambientColor = light.ambientColor;
					pointLightSSBO[p].diffuseColor = light.diffuseColor;
					pointLightSSBO[p].constant = light.constantAttenuation;
					pointLightSSBO[p].linear = light.linearAttenuation;
					pointLightSSBO[p].quadratic = light.quadraticAttenuation;
					pointLightSSBO[p].specularStrength = light.specularStrength;
					pointLightSSBO[p].shininess = light.shininess;
					pointLightSSBO[p].shadowmapIndex = shadowIndex;
					p++;
					break;
				case LightType::DIRECTIONAL:
					dirLightSSBO[d].ambientColor = light.ambientColor;
					dirLightSSBO[d].diffuseColor = light.diffuseColor;
					dirLightSSBO[d].direction = light.direction;
					dirLightSSBO[d].specularStrength = light.specularStrength;
					dirLightSSBO[d].shininess = light.shininess;
					dirLightSSBO[d].shadowmapIndex = shadowIndex;
					d++;
					break;
				case LightType::SPOT:
					spotLightSSBO[s].position = m_world->GetComponent<TransformComponent>(entity).position;
					spotLightSSBO[s].direction = light.direction;
					spotLightSSBO[s].ambientColor = light.ambientColor;
					spotLightSSBO[s].diffuseColor = light.diffuseColor;
					spotLightSSBO[s].innerCutoff = glm::cos(glm::radians(light.innerCutoffAngle));
					spotLightSSBO[s].outerCutoff = glm::cos(glm::radians(light.outerCutoffAngle));
					spotLightSSBO[s].constant = light.constantAttenuation;
					spotLightSSBO[s].linear = light.linearAttenuation;
					spotLightSSBO[s].quadratic = light.quadraticAttenuation;
					spotLightSSBO[s].specularStrength = light.specularStrength;
					spotLightSSBO[s].shininess = light.shininess;
					spotLightSSBO[s].shadowmapIndex = shadowIndex;
					s++;
					break;
				}
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().dirLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().spotLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceMultiBuffer.allocation);

			GPULightStatsData lightStats;
			lightStats.numPLights = p;
			lightStats.numDLights = d;
			lightStats.numSLights = s;

			GPULightIndexData lightIndexData;
			lightIndexData.lightSpaceIndex = lightIndex;

			void* data;

			// Map camera data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation, &data);
			memcpy(data, &camera.data, sizeof(ViewData));
			vmaUnmapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation);

			// Map Light Space Index to Buffer
			vmaMapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation, &data);
			memcpy(data, &lightIndexData, sizeof(GPULightIndexData));
			vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation);

			int meshIndex = 0;
			Material* lastMaterial = nullptr;

			// Render each mesh
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = m_world->GetComponent<MeshComponent>(entity);

				if (!m_world->GetComponentFlag<MeshComponent, FlagDirty>(entity) &&
					!m_world->GetComponentFlag<MeshComponent, FlagDeleted>(entity))
				{

					// Bind material pipeline if it does not match previous material
					if (&mesh.material != lastMaterial);
					{
						vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipeline);
						lastMaterial = &mesh.material;

						// Set Pipeline Viewport
						VkViewport viewport = {};
						viewport.x = 0.0f;
						viewport.y = 0.0f;
						viewport.width = (float)offscreenExtent.width;
						viewport.height = (float)offscreenExtent.height;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;

						vkCmdSetViewport(cmd, 0, 1, &viewport);

						VkRect2D scissor = {};
						scissor.offset = { 0, 0 };
						scissor.extent = offscreenExtent;

						vkCmdSetScissor(cmd, 0, 1, &scissor);

						// Bind Camera View/Proj Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 0, 1, &GetCurrentFrame().cameraViewProjDescriptor, 0, nullptr);

						// Bind Object Data Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 1, 1, &GetCurrentFrame().objectDescriptor, 0, nullptr);

						// Bind Light Space Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 2, 1, &GetCurrentFrame().lightSpaceMultiDescriptor, 0, nullptr);

						// Bind Camera Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 3, 1, &GetCurrentFrame().cameraDescriptor, 0, nullptr);

						// Bind Light Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 4, 1, &GetCurrentFrame().lightDescriptor, 0, nullptr);

						// Shadowmap Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 5, 1, &GetCurrentFrame().shadowmapDescriptor, 0, nullptr);

						// Texture Descriptor
						vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
							mesh.material.pipelineLayout, 6, 1, &mesh.material.textureSet, 0, nullptr);
					}

					// Bind Vertices, Indices and Descriptor Sets
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offsets);
					vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

					// Draw Indexed Vertices
					vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indexCount), 1, 0, 0, meshIndex);

					meshIndex++;
				}
			}
		}

		void VulkanEngine::DrawDebugObjects(VkCommandBuffer cmd, uint32_t index)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline);

			// Set Pipeline Viewport
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)offscreenExtent.width;
			viewport.height = (float)offscreenExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = offscreenExtent;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			// Bind Camera View/Proj Descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				debugPipelineLayout, 0, 1, &GetCurrentFrame().cameraViewProjDescriptor, 0, nullptr);

			// Bind Vertex Buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &GetCurrentFrame().debugVertexBuffer.buffer, offsets);

			// Map Indirect Commands to buffer
			void* data;
			vmaMapMemory(allocator, GetCurrentFrame().debugIndirectCommandsBuffer.allocation, &data);
			memcpy(data, GetCurrentFrame().debugIndirectCommands.data(), sizeof(VkDrawIndexedIndirectCommand) * GetCurrentFrame().debugIndirectCommands.size());
			vmaUnmapMemory(allocator, GetCurrentFrame().debugIndirectCommandsBuffer.allocation);

			// Bind Index Buffer
			vkCmdBindIndexBuffer(cmd, GetCurrentFrame().debugIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			
			uint32_t draw_count = GetCurrentFrame().debugIndirectCommands.size();
			uint32_t draw_stride = sizeof(VkDrawIndexedIndirectCommand);

			// Draw Debug Lines/Boxes Using Draw Indirect
			vkCmdDrawIndexedIndirect(cmd, GetCurrentFrame().debugIndirectCommandsBuffer.buffer, 0, draw_count, draw_stride);
		}

		void VulkanEngine::MapObjectData()
		{
			// Map all object data to storage buffer
			void* objectData;
			vmaMapMemory(allocator, GetCurrentFrame().objectBuffer.allocation, &objectData);

			GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

			int i = 0;

			// For each mesh entity, calculate its object data and map to storage buffer
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				// Update object data
				objectSSBO[i].model = BuildMeshTransform(m_world->GetComponent<TransformComponent>(entity));
				objectSSBO[i].inv_model = glm::inverse(objectSSBO[i].model);

				i++;
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().objectBuffer.allocation);
		}

		glm::mat4 VulkanEngine::BuildMeshTransform(TransformComponent transform)
		{
			// Set Translation
			glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), (glm::vec3)transform.position);

			// Set Rotation
			model_transform = glm::rotate(model_transform, glm::radians(float(transform.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(float(transform.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(float(transform.rotation.z)), glm::vec3(0.0f, 0.0f, -1.0f));

			// Set Scale
			model_transform = glm::scale(model_transform, (glm::vec3)transform.scale);

			return model_transform;
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Cleanup()
		{
			if (isInitialized)
			{
				// Make sure GPU has stopped working
				vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, 1000000000);

				// Cleanup Deferred Renderer
				deferredRenderer.Cleanup();

				// Flush all queues, destorying all created resources
				mainDeletionQueue.flush();
				swapchainDeletionQueue.flush();
				offscreenDeletionQueue.flush();

				Stop();

				// Cleanup Allocator/Cache
				descriptorAllocator->Cleanup();
				descriptorLayoutCache->Cleanup();

				vkDestroyDevice(device, nullptr);
				vkDestroySurfaceKHR(instance, surface, nullptr);
				vkDestroyInstance(instance, nullptr);
			}
		}

		//-------------------------------------------------------------------------------------

		AllocatedBuffer VulkanEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags)
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
			VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));

			return newBuffer;
		}

		void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
		{
			// Allocate Default command buffer that will be used for instant commands
			VkCommandBufferAllocateInfo cmdAllocInfo = VKInit::CommandBufferAllocateInfo(uploadContext.commandPool, 1);

			VkCommandBuffer cmd;
			VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

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

			VK_CHECK(vkWaitForFences(device, 1, &uploadContext.uploadFence, true, 9999999999));
			VK_CHECK(vkResetFences(device, 1, &uploadContext.uploadFence));

			// Clear command pool and free command buffer
			VK_CHECK(vkResetCommandPool(device, uploadContext.commandPool, 0));
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::DrawDebugLine(Debug::Line line)
		{
			// Create debug line vertices to current frames vertices vector
			Vertex startVertex, endVertex;
			startVertex.pos = line.start;
			startVertex.color = line.color;
			startVertex.normal = Vector3(0.0f, 0.0f, 0.0f);
			startVertex.uv = Vector2(0.0f, 0.0f);

			endVertex.pos = line.end;
			endVertex.color = line.color;
			endVertex.normal = Vector3(0.0f, 0.0f, 0.0f);
			endVertex.uv = Vector2(0.0f, 0.0f);

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

		void VulkanEngine::DrawDebugBox(Debug::Box box)
		{
			const int numVertices = 4;
			const int numIndices = 8;

			const int firstVertex = GetCurrentFrame().debugVertices.size();
			const int firstIndex = GetCurrentFrame().debugIndices.size();

			Vertex vert = {};
			vert.color = box.color;
			vert.normal = Vector3f(0.0f, 0.0f, 0.0f);
			vert.uv = Vector2(0.0f, 0.0f);

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

		void VulkanEngine::DrawDebugCube(Debug::Cube cube)
		{
			const int numVertices = 8;
			const int numIndices = 24;

			int firstVertex = GetCurrentFrame().debugVertices.size();
			int firstIndex = GetCurrentFrame().debugIndices.size();

			Vertex vert = {};
			vert.color = cube.color;
			vert.normal = Vector3(0.0f, 0.0f, 0.0f);
			vert.uv = Vector2(0.0f, 0.0f);

			// Add Vertices to vector
			for (int i = 0; i < numVertices; i++)
			{
				vert.pos = cube.origin + (cube.halfSize * cubePositions[i]);
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