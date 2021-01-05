#include "VulkanEngine.h"

#include "VKInitializers.h"
#include "VKDescriptors.h"
#include "ModelLoader.h"
#include "VKTexture.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>
#include <string>

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
		GLFWwindow* VulkanEngine::Init(UI::UIManager* UIManager)
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

			windowExtent.width = WIDTH;
			windowExtent.height = HEIGHT;

			// Initialize Offscreen Variables with Default Values
			offscreenExtent.width = 1024;
			offscreenExtent.height = 1024;
			offscreenFormat = VK_FORMAT_R8G8B8A8_SRGB;

			viewportSize = ImVec2(0.0f, 0.0f);
			offscreenInitialized = false;

			// Initialize Shadowmap Resolution/Format
			shadowExtent.width = 1024;
			shadowExtent.height = 1024;
			shadowFormat = VK_FORMAT_D16_UNORM;

			window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Puffin Engine", monitor, nullptr);

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

			// Initialize Descriptor Sets
			InitDescriptors();

			// Initialize Pipelines
			InitPipelines();

			InitShadowPipeline();

			InitTextureSampler();

			InitDepthSampler();

			// Initialize All Scene Objects
			InitScene();

			InitShadowmapDescriptors();

			// Pass Camera to UI
			UIManager->GetWindowSettings()->SetCamera(&camera);

			// Initialize ImGui
			InitImGui();

			InitImGuiTextureIDs();
			InitShadowTextureIDs();

			UIManager->GetWindowViewport()->SetTextureSampler(textureSampler);

			isInitialized = true;

			return window;
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

			VkPhysicalDeviceDescriptorIndexingFeatures supportedDescriptorFeatures = {};
			supportedDescriptorFeatures.runtimeDescriptorArray = VK_TRUE;

			// Select GPU with VK Bootstrap
			// We want a gpu which can write to glfw surface and supports vulkan 1.2
			vkb::PhysicalDeviceSelector selector{ vkb_inst };
			vkb::PhysicalDevice physicalDevice = selector
				.set_minimum_version(1, 2)
				.set_surface(surface)
				.set_required_features(supportedFeatures)
				.add_desired_extension("VK_EXT_debug_marker")
				.select()
				.value();

			// Create Final Vulkan Device
			vkb::DeviceBuilder deviceBuilder{ physicalDevice };
			vkb::Device vkbDevice = deviceBuilder
				.add_pNext(&supportedDescriptorFeatures)
				.build()
				.value();

			// Get VKDevice handle used in rest of vulkan application
			device = vkbDevice.device;
			chosenGPU = physicalDevice.physical_device;

			// Use vkbootstrap to get a graphics queue
			graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
			graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

			// Initialize Memory Allocator
			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = chosenGPU;
			allocatorInfo.device = device;
			allocatorInfo.instance = instance;

			// Create Allocator
			vmaCreateAllocator(&allocatorInfo, &allocator);
		}

		void VulkanEngine::InitSwapchain()
		{
			vkb::SwapchainBuilder swapchainBuilder{ chosenGPU, device, surface };

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

			// Set Debug Name for RenderDoc
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;

			for (int i = 0; i < swapchain_imagecount; i++)
			{
				// Create Image
				vmaCreateImage(allocator, &imageInfo, &imageAllocInfo,
					&offscreenAttachments[i].image, &offscreenAttachments[i].allocation, nullptr);

				nameInfo.object = (uint64_t)offscreenAttachments[i].image;
				std::string string = "Offscreen Framebuffer" + std::to_string(i);
				nameInfo.pObjectName = string.c_str();
				vkDebugMarkerSetObjectNameEXT(device, &nameInfo);

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
				VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].guiCommandPool));
				VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].shadowCommandPool));

				// Allocate Default Command Buffer that we will use for scene rendering
				VkCommandBufferAllocateInfo allocInfo = VKInit::CommandBufferAllocateInfo(frames[i].commandPool, 1); 

				// Allocate GUI Command Buffer used for rendering UI
				VkCommandBufferAllocateInfo allocInfoGui = VKInit::CommandBufferAllocateInfo(frames[i].guiCommandPool, 1);

				VkCommandBufferAllocateInfo allocInfoShadow = VKInit::CommandBufferAllocateInfo(frames[i].shadowCommandPool, 1);

				// Allocate buffers
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].mainCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfoGui, &frames[i].guiCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfoShadow, &frames[i].shadowCommandBuffer));

				// Push destruction of both command pools/buffers to deletion queue
				mainDeletionQueue.push_function([=]() {
					vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
					vkDestroyCommandPool(device, frames[i].guiCommandPool, nullptr);
					vkDestroyCommandPool(device, frames[i].shadowCommandPool, nullptr);
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
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, MAX_LIGHTS * 3);

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

				// Create Camera View/Proj Buffer
				frames[i].cameraViewProjBuffer = CreateBuffer(sizeof(GPUCameraData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Allocate Camera View/Proj Descriptor Set
				VkDescriptorBufferInfo cameraBufferInfo;
				cameraBufferInfo.buffer = frames[i].cameraViewProjBuffer.buffer;
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(GPUCameraData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &cameraBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].cameraViewProjDescriptor, cameraViewProjSetLayout);

				// Create Object Storage Buffers
				const int MAX_OBJECTS = 10000;
				frames[i].objectBuffer = CreateBuffer(sizeof(GPUObjectData) * MAX_OBJECTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Allocate Object Descriptor Set
				VkDescriptorBufferInfo objectBufferInfo;
				objectBufferInfo.buffer = frames[i].objectBuffer.buffer;
				objectBufferInfo.offset = 0;
				objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.Build(frames[i].objectDescriptor, objectSetLayout);

				// Create Descriptor Sets for Fragment Shader Stage

				// Create Camera/Light Buffers
				frames[i].cameraBuffer = CreateBuffer(sizeof(ViewData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				frames[i].pointLightBuffer = CreateBuffer(sizeof(GPUPointLightData) * MAX_LIGHTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].directionalLightBuffer = CreateBuffer(sizeof(GPUDirLightData) * MAX_LIGHTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].spotLightBuffer = CreateBuffer(sizeof(GPUSpotLightData) * MAX_LIGHTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].lightStatsBuffer = CreateBuffer(sizeof(LightStatsData),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

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
				pointLightInfo.range = sizeof(GPUPointLightData) * MAX_LIGHTS;

				VkDescriptorBufferInfo directionalLightInfo;
				directionalLightInfo.buffer = frames[i].directionalLightBuffer.buffer;
				directionalLightInfo.offset = 0;
				directionalLightInfo.range = sizeof(GPUDirLightData) * MAX_LIGHTS;

				VkDescriptorBufferInfo spotLightInfo;
				spotLightInfo.buffer = frames[i].spotLightBuffer.buffer;
				spotLightInfo.offset = 0;
				spotLightInfo.range = sizeof(GPUSpotLightData) * MAX_LIGHTS;

				VkDescriptorBufferInfo lightStatInfo;
				lightStatInfo.buffer = frames[i].lightStatsBuffer.buffer;
				lightStatInfo.offset = 0;
				lightStatInfo.range = sizeof(LightStatsData);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &pointLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(1, &directionalLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(2, &spotLightInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindBuffer(3, &lightStatInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frames[i].lightDescriptor, lightSetLayout);

				// Create Light Space Buffer for Shadow Vertex Stage
				frames[i].lightSpaceBuffer = CreateBuffer(sizeof(GPULightSpaceData) * MAX_LIGHTS,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				frames[i].lightSpaceIndexBuffer = CreateBuffer(sizeof(int),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

				// Initialize Light Space Descriptor Layout for Shadow/Shader Vertex Stage
				VkDescriptorBufferInfo lightSpaceInfo;
				lightSpaceInfo.buffer = frames[i].lightSpaceBuffer.buffer;
				lightSpaceInfo.offset = 0;
				lightSpaceInfo.range = sizeof(GPULightSpaceData) * MAX_LIGHTS;

				VkDescriptorBufferInfo lightSpaceIndexInfo;
				lightSpaceIndexInfo.buffer = frames[i].lightSpaceIndexBuffer.buffer;
				lightSpaceIndexInfo.offset = 0;
				lightSpaceIndexInfo.range = sizeof(int);

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindBuffer(0, &lightSpaceInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.BindBuffer(1, &lightSpaceIndexInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frames[i].lightSpaceDescriptor, lightSpaceSetLayout);
			}
		}

		void VulkanEngine::InitPipelines()
		{
			// Read Shader Code from files
			auto vertShaderCode = ReadFile("shaders/vert.spv");
			auto fragShaderCode = ReadFile("shaders/frag.spv");

			// Create Shader Modules from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				cameraViewProjSetLayout,
				objectSetLayout,
				lightSpaceSetLayout,
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
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			pipelineBuilder.colorBlendAttachment = VKInit::ColorBlendAttachmentState();

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Assign Pipeline Layout to Pipeline
			pipelineBuilder.pipelineLayout = meshMaterial.pipelineLayout;

			// Build Pipeline
			meshMaterial.pipeline = pipelineBuilder.build_pipeline(device, renderPass);
		}

		void VulkanEngine::InitShadowPipeline()
		{
			// Read Shader Code from files
			auto vertShaderCode = ReadFile("shaders/shadowmap_vert.spv");
			//auto fragShaderCode = ReadFile("shaders/shadowmap_frag.spv");

			// Create Shader Module from code
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			//VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

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
			//pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(glm::vec3);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription attributeDescription = {};
			attributeDescription.binding = 0;
			attributeDescription.location = 0;
			attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescription.offset = offsetof(Vertex, Vertex::pos);

			//auto bindingDescription = Vertex::getBindingDescription();
			//auto attributeDescriptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescription);

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
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

			// Multisampling
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			pipelineBuilder.colorBlendAttachment = VKInit::ColorBlendAttachmentState();

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Assign Pipeline Layout
			pipelineBuilder.pipelineLayout = shadowPipelineLayout;

			// Build Pipeline
			shadowPipeline = pipelineBuilder.build_pipeline(device, renderPassShadows);
		}

		void VulkanEngine::InitScene()
		{
			// Initialize Camera
			camera.position = glm::vec3(0.0f, 0.0f, 10.0f);
			camera.direction = glm::vec3(0.0f, 0.0f, -1.0f);
			camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
			camera.fov = 60.0f;
			camera.aspect = (float)offscreenExtent.width / (float)offscreenExtent.height;
			camera.zNear = 0.1f;
			camera.zFar = 100.0f;
			InitCamera(camera);

			// Initialize Lights
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = world->GetComponent<LightComponent>(entity);

				InitLight(light);
			}

			// Initialize Meshes
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);
				InitMesh(mesh);
			}
		}

		

		void VulkanEngine::InitMesh(MeshComponent& mesh)
		{
			// Load Texture Data
			Util::LoadImageFromFile(*this, mesh.texture_path.c_str(), mesh.texture);

			VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_UNORM, mesh.texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &mesh.texture.imageView));

			// Load Mesh Data
			IO::LoadMesh(mesh);

			// Init Mesh Buffers
			InitVertexBuffer(mesh);
			InitIndexBuffer(mesh);

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

				// Create Image View
				VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(shadowFormat,
					light.depthAttachments[i].image, VK_IMAGE_ASPECT_DEPTH_BIT);

				VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &light.depthAttachments[i].imageView));

				// Add to deletion queues
				offscreenDeletionQueue.push_function([=]()
				{
					vkDestroyImageView(device, light.depthAttachments[i].imageView, nullptr);
					vmaDestroyImage(allocator, light.depthAttachments[i].image, light.depthAttachments[i].allocation);
				});

				// Create Framebuffer
				framebufferInfo.pAttachments = &light.depthAttachments[i].imageView;
				framebufferInfo.attachmentCount = 1;

				VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &light.depthFramebuffers[i]));

				offscreenDeletionQueue.push_function([=]()
				{
					vkDestroyFramebuffer(device, light.depthFramebuffers[i], nullptr);
				});
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

			camera.yaw = -90.0f;
			camera.speed = 5.0f;

			// Calculate Camera View Matrix
			camera.matrices.view = glm::lookAt(camera.position, camera.lookat, camera.up);
		}

		void VulkanEngine::InitVertexBuffer(MeshComponent& mesh)
		{
			// Copy Loaded Mesh data into mesh vertex buffer
			const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, mesh.vertices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
			mesh.vertexBuffer = CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);
			});

			// Add destruction of vertex buffer to deletion queue
			offscreenDeletionQueue.push_function([=]()
			{
				vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanEngine::InitIndexBuffer(MeshComponent& mesh)
		{
			// Copy Loaded Index data into mesh index buffer
			const size_t bufferSize = mesh.indices.size() * sizeof(uint32_t);

			// Allocated Staging Buffer - Map Indices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map Index data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, mesh.indices.data(), bufferSize);
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Allocate Index Buffer - Transfer Indices into GPU Memory
			mesh.indexBuffer = CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			//Copy from CPU Memory to GPU Memory
			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.indexBuffer.buffer, 1, &copy);
			});

			// Add destruction of index buffer to deletion queue
			offscreenDeletionQueue.push_function([=]()
			{
				vmaDestroyBuffer(allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
			});

			// CLeanup Staging Buffer
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
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
			VkSamplerCreateInfo samplerInfo = VKInit::SamplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
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

		void VulkanEngine::InitShadowmapDescriptors()
		{
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
					LightComponent& light = world->GetComponent<LightComponent>(entity);

					shadowmapBufferInfo.imageView = light.depthAttachments[i].imageView;
					imageInfos.push_back(shadowmapBufferInfo);
				}

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindImages(0, imageInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
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
			init_info.PhysicalDevice = chosenGPU;
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

		void VulkanEngine::InitShadowTextureIDs()
		{
			shadowTextureIDs.clear();
			shadowTextureIDs = std::vector<ImTextureID>(FRAME_OVERLAP);

			LightComponent& light = world->GetComponent<LightComponent>(4);
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				shadowTextureIDs[i] = ImGui_ImplVulkan_AddTexture(depthSampler, light.depthAttachments[i].imageView,
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
			// Update Offscreen Extents
			offscreenExtent.width = viewportSize.x;
			offscreenExtent.height = viewportSize.y;

			// Delete all Offscreen Variables in deletion queue
			offscreenDeletionQueue.flush();

			// Initialize Offscreen Variables and Scene
			InitOffscreen();
			InitOffscreenFramebuffers();
			InitImGuiTextureIDs();
			InitPipelines();
			InitScene();
			InitShadowTextureIDs();
			InitShadowmapDescriptors();

			offscreenInitialized = true;
		}

		//-------------------------------------------------------------------------------------

		bool VulkanEngine::Update(UI::UIManager* UIManager, Input::InputManager* InputManager, float dt)
		{
			glfwPollEvents();

			bool running = UIManager->DrawUI(dt, InputManager);

			UpdateCamera(camera, InputManager, dt);

			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);

				// Initialize/Re-Init Mesh
				if (mesh.flag_created)
				{
					InitMesh(mesh);
					mesh.flag_created = false;
				}

				// Remove Mesh
				if (mesh.flag_deleted)
				{
					world->RemoveComponent<MeshComponent>(entity);
					mesh.flag_deleted = false;
				}
			}

			DrawFrame(UIManager);

			return running;
		}

		void VulkanEngine::UpdateCamera(CameraComponent& camera, Puffin::Input::InputManager* inputManager, float delta_time)
		{
			if (inputManager->IsCursorLocked())
			{
				// Camera Movement
				if (inputManager->GetAction("CamMoveLeft").state == Puffin::Input::HELD)
				{
					camera.position += camera.speed * camera.right * delta_time;
				}
				else if (inputManager->GetAction("CamMoveRight").state == Puffin::Input::HELD)
				{
					camera.position -= camera.speed * camera.right * delta_time;
				}

				if (inputManager->GetAction("CamMoveForward").state == Puffin::Input::HELD)
				{
					camera.position += camera.speed * camera.direction * delta_time;
				}
				else if (inputManager->GetAction("CamMoveBackward").state == Puffin::Input::HELD)
				{
					camera.position -= camera.speed * camera.direction * delta_time;
				}

				if (inputManager->GetAction("CamMoveUp").state == Puffin::Input::HELD)
				{
					camera.position += camera.speed * camera.up * delta_time;
				}
				else if (inputManager->GetAction("CamMoveDown").state == Puffin::Input::HELD)
				{
					camera.position -= camera.speed * camera.up * delta_time;
				}

				// Mouse Rotation
				camera.yaw += inputManager->GetMouseXOffset();
				camera.pitch -= inputManager->GetMouseYOffset();

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

			// Recalculate camera perspective if fov has changed, store new fov in prevFov
			if (camera.fov != camera.prevFov)
			{
				camera.matrices.perspective = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);
				camera.prevFov = camera.fov;
			}

			camera.matrices.view = glm::lookAt(camera.position, camera.lookat, camera.up);
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::DrawFrame(UI::UIManager* UIManager)
		{
			// Wait until gpu has finished rendering last frame. Timeout of 1 second
			VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence)); // Reset fence
			
			// Request image from swapchain
			uint32_t swapchainImageIndex;
			VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 0, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

			// Now that we are sure commands are finished executing, reset command buffer
			VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().mainCommandBuffer, 0));
			VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().guiCommandBuffer, 0));

			// Pass Offscreen Framebuffer to Viewport Window and Render Viewport
			if (offscreenInitialized)
				UIManager->GetWindowViewport()->Draw(viewportTextureIDs[swapchainImageIndex]);
				//UIManager->GetWindowViewport()->Draw(shadowTextureIDs[frameNumber % FRAME_OVERLAP]);
			else
				UIManager->GetWindowViewport()->DrawWithoutImage();

			viewportSize = UIManager->GetWindowViewport()->GetViewportSize();

			// Draw ImGui
			ImGui::Render();

			// Recreate Swapchain if window size changes
			if (framebufferResized)
			{
				RecreateSwapchain();
			}

			// Recreate Viewport if it size changes
			if (viewportSize.x != offscreenExtent.width ||
				viewportSize.y != offscreenExtent.height)
			{
				RecreateOffscreen();
			}

			// Record Command Buffers
			VkCommandBuffer cmdShadows = RecordShadowCommandBuffers(swapchainImageIndex);
			VkCommandBuffer cmdMain = RecordMainCommandBuffers(swapchainImageIndex);
			VkCommandBuffer cmdGui = RecordGUICommandBuffer(swapchainImageIndex);

			std::array<VkCommandBuffer, 3> submitCommandBuffers = { cmdShadows, cmdMain, cmdGui };

			// Prepare the submission into graphics queue
			// we will signal the _renderSemaphore, to signal that rendering has finished
			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submit.pWaitDstStageMask = &waitStage;

			submit.waitSemaphoreCount = 1;
			submit.pWaitSemaphores = &GetCurrentFrame().presentSemaphore;

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

			submit.commandBufferCount = submitCommandBuffers.size();
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

			// Number of Frames that have completed rendering
			frameNumber++;
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

			// Render Depth Map for each Light Source
			//RenderShadowPass(cmd, index);

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

			MapObjectData();

			// Bind Pipeline
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

			// Fill Light Space Buffer
			void* lightSpaceData;
			vmaMapMemory(allocator, GetCurrentFrame().lightSpaceBuffer.allocation, &lightSpaceData);

			GPULightSpaceData* lightSpaceSSBO = (GPULightSpaceData*)lightSpaceData;

			int lightIndex = 0;
			
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = world->GetComponent<LightComponent>(entity);
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.castShadows && light.type == LightType::SPOT)
				{
					// Near/Far Plane to render depth within
					float near_plane = 1.0f;
					float far_plane = 100.0f;

					// Calculate Light Space Projection Matrix
					glm::mat4 lightProj = glm::perspective(
						glm::radians(45.0f),
						(float)shadowExtent.width / (float)shadowExtent.height, 
						near_plane, far_plane);

					glm::mat4 lightView = glm::lookAt(
						glm::vec3(transform.position),
						glm::vec3(transform.position + light.direction),
						glm::vec3(0.0f, 1.0f, 0.0f));

					lightSpaceSSBO[lightIndex].lightSpaceMatrix = lightView * lightProj;

					lightIndex++;
				}
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceBuffer.allocation);

			lightIndex = 0;

			// For Each Shadowcasting Light Source
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& light = world->GetComponent<LightComponent>(entity);

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.castShadows && light.type == LightType::SPOT)
				{
					TransformComponent& transform = world->GetComponent<TransformComponent>(entity);

					shadowRPInfo.framebuffer = light.depthFramebuffers[frameNumber % FRAME_OVERLAP];

					// Begin Shadow Render Pass
					vkCmdBeginRenderPass(cmd, &shadowRPInfo, VK_SUBPASS_CONTENTS_INLINE);

					int lightSpaceIndex = lightIndex;

					void* data;
					vmaMapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation, &data);
					memcpy(data, &lightSpaceIndex, sizeof(int));
					vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation);

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
						MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);

						// Bind Vertices and Indices
						VkDeviceSize offsets[] = { 0 };
						vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offsets);
						vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

						// Draw Indexed Vertices
						vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, meshIndex);

						meshIndex++;
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
			// This is already done in ShadowPass so not needed here
			//MapObjectData();

			// Map all light data to storage buffer
			void* pointLightData;
			void* dirLightData;
			void* spotLightData;

			vmaMapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation, &pointLightData);
			vmaMapMemory(allocator, GetCurrentFrame().directionalLightBuffer.allocation, &dirLightData);
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
				LightComponent& light = world->GetComponent<LightComponent>(entity);

				int shadowIndex = -1;

				// Only render depth map if light is set to cast shadows and is spotlight type
				if (light.castShadows && light.type == LightType::SPOT)
				{
					shadowIndex = lightIndex;
					lightIndex++;
				}

				switch (light.type)
				{
				case LightType::POINT:
					pointLightSSBO[p].position = world->GetComponent<TransformComponent>(entity).position;
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
					spotLightSSBO[s].position = world->GetComponent<TransformComponent>(entity).position;
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

				lightIndex++;
			}

			vmaUnmapMemory(allocator, GetCurrentFrame().pointLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().directionalLightBuffer.allocation);
			vmaUnmapMemory(allocator, GetCurrentFrame().spotLightBuffer.allocation);

			LightStatsData lightStats;
			lightStats.numPLights = p;
			lightStats.numDLights = d;
			lightStats.numSLights = s;

			int lightSpaceIndex = lightIndex;

			void* data;

			GPUCameraData cameraData;
			cameraData.view = camera.matrices.view;
			cameraData.proj = camera.matrices.perspective;
			cameraData.proj[1][1] *= -1;
			
			// Map camera view/proj data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().cameraViewProjBuffer.allocation, &data);
			memcpy(data, &cameraData, sizeof(GPUCameraData));
			vmaUnmapMemory(allocator, GetCurrentFrame().cameraViewProjBuffer.allocation);

			// Map camera data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation, &data);
			memcpy(data, &camera.data, sizeof(ViewData));
			vmaUnmapMemory(allocator, GetCurrentFrame().cameraBuffer.allocation);

			// Map Light stats data to uniform buffer
			vmaMapMemory(allocator, GetCurrentFrame().lightStatsBuffer.allocation, &data);
			memcpy(data, &lightStats, sizeof(LightStatsData));
			vmaUnmapMemory(allocator, GetCurrentFrame().lightStatsBuffer.allocation);

			// Map Light Space Index to Buffer
			vmaMapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation, &data);
			memcpy(data, &lightSpaceIndex, sizeof(int));
			vmaUnmapMemory(allocator, GetCurrentFrame().lightSpaceIndexBuffer.allocation);

			int meshIndex = 0;
			Material* lastMaterial = nullptr;

			// Render each mesh
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);

				// Bind material pipeline if it does not match previous material
				if (&mesh.material != lastMaterial);
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						mesh.material.pipeline);
					lastMaterial = &mesh.material;

					// Bind Camera View/Proj Descriptor
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						mesh.material.pipelineLayout, 0, 1, &GetCurrentFrame().cameraViewProjDescriptor, 0, nullptr);

					// Bind Object Data Descriptor
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						mesh.material.pipelineLayout, 1, 1, &GetCurrentFrame().objectDescriptor, 0, nullptr);

					// Bind Light Space Descriptor
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						mesh.material.pipelineLayout, 2, 1, &GetCurrentFrame().lightSpaceDescriptor, 0, nullptr);

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
				vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, meshIndex);

				meshIndex++;
			}
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
				objectSSBO[i].model = BuildMeshTransform(world->GetComponent<TransformComponent>(entity));
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
			model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

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

				// Flush all queues, destorying all created resources
				mainDeletionQueue.flush();
				swapchainDeletionQueue.flush();
				offscreenDeletionQueue.flush();

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

			vkWaitForFences(device, 1, &uploadContext.uploadFence, true, 9999999999);
			vkResetFences(device, 1, &uploadContext.uploadFence);

			// Clear command pool and free command buffer
			vkResetCommandPool(device, uploadContext.commandPool, 0);
		}
	}
}