#include "VulkanEngine.h"

#include "VKInitializers.h"
#include "ModelLoader.h"
#include "VKTexture.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>

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

			window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Puffin Engine", monitor, nullptr);

			//glfwMaximizeWindow(window);
			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

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

			// Initialise Swapchain Framebuffers
			InitFramebuffers();

			// Initialise Semaphores and Fences
			InitSyncStructures();

			// Initialize Descriptor Sets
			InitDescriptors();

			// Initialize Pipelines
			InitPipelines();

			// Initialize All Scene Objects
			InitScene();

			// Pass Camera to UI
			UIManager->GetWindowSettings()->SetCamera(&camera);

			// Initialize ImGui
			InitImGui();

			InitTextureSampler();

			UIManager->GetWindowViewport()->SetTextureSampler(textureSampler);

			InitDescriptorSets();

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

			// Select GPU with VK Bootstrap
			// We want a gpu which can write to glfw surface and supports vulkan 1.2
			vkb::PhysicalDeviceSelector selector{ vkb_inst };
			vkb::PhysicalDevice physicalDevice = selector
				.set_minimum_version(1, 2)
				.set_surface(surface)
				.set_required_features(supportedFeatures)
				.select()
				.value();

			// Create Final Vulkan Device
			vkb::DeviceBuilder deviceBuilder{ physicalDevice };
			vkb::Device vkbDevice = deviceBuilder.build().value();

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
				//use vsync present mode
				.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
				.set_desired_extent(windowExtent.width, windowExtent.height)
				.build()
				.value();

			// Store Swapchain and related images/views
			swapchain = vkbSwapchain.swapchain;

			for (int i = 0; i < vkbSwapchain.image_count; i++)
			{
				AllocatedImage swapchainAttachment;
				swapchainAttachment.image = vkbSwapchain.get_images().value()[i];
				swapchainAttachment.imageView = vkbSwapchain.get_image_views().value()[i];
				swapchainAttachments.push_back(swapchainAttachment);
			}

			swapchainImageFormat = vkbSwapchain.image_format;

			mainDeletionQueue.push_function([=]()
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
			VkImageCreateInfo imageInfo = vkinit::image_create_info(offscreenFormat, 
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);
			VmaAllocationCreateInfo imageAllocInfo = {};
			imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			for (int i = 0; i < swapchain_imagecount; i++)
			{
				// Create Image
				vmaCreateImage(allocator, &imageInfo, &imageAllocInfo,
					&offscreenAttachments[i].image, &offscreenAttachments[i].allocation, nullptr);

				// Create Image View
				VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(offscreenFormat,
					offscreenAttachments[i].image, VK_IMAGE_ASPECT_COLOR_BIT);

				VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &offscreenAttachments[i].imageView));

				// Add to deletion queues
				mainDeletionQueue.push_function([=]()
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
			VkImageCreateInfo depthImageInfo = vkinit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

			// Allocate depth image from local gpu memory
			VmaAllocationCreateInfo depthImageAllocInfo = {};
			depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Allocate and create image
			vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthAttachment.image, &depthAttachment.allocation, nullptr);

			// Build Image View for depth image to use in rendering
			VkImageViewCreateInfo depthImageViewInfo = vkinit::imageview_create_info(depthFormat, depthAttachment.image, VK_IMAGE_ASPECT_DEPTH_BIT);

			VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthAttachment.imageView));

			// Add to deletion queues
			mainDeletionQueue.push_function([=]()
			{
				vkDestroyImageView(device, depthAttachment.imageView, nullptr);
				vmaDestroyImage(allocator, depthAttachment.image, depthAttachment.allocation);
			});
		}

		void VulkanEngine::InitCommands()
		{
			// Create Command Pool for commands submitted to graphics queue
			// we also want the pool to allow for resetting individual command buffers
			VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			// Allocate Command Pool/Buffer for each frame data struct
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				// Allocate Command Pools
				VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));
				VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].guiCommandPool));

				// Allocate Default Command Buffer that we will use for scene rendering
				VkCommandBufferAllocateInfo allocInfo = vkinit::command_buffer_allocate_info(frames[i].commandPool, 1); 

				// Allocate GUI Command Buffer used for rendering UI
				VkCommandBufferAllocateInfo allocInfoGui = vkinit::command_buffer_allocate_info(frames[i].guiCommandPool, 1);

				// Allocate buffers
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frames[i].mainCommandBuffer));
				VK_CHECK(vkAllocateCommandBuffers(device, &allocInfoGui, &frames[i].guiCommandBuffer));

				// Push destruction of both command pools/buffers to deletion queue
				mainDeletionQueue.push_function([=]() {
					vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
					vkDestroyCommandPool(device, frames[i].guiCommandPool, nullptr);
				});
			}

			// Create Upload Command Pool
			VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily);

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

			// after renderpass ends, the imahe has to be on a layout ready for display
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

			// Create Renderpass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			// Connect color attachment
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			
			// Connect Subpass
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;

			VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPassGUI));

			mainDeletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(device, renderPassGUI, nullptr);
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
			framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);
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

				// Attach swapchain image view to Framebuffer
				fb_gui_info.pAttachments = &swapchainAttachments[i].imageView;
				fb_gui_info.attachmentCount = 1;

				VK_CHECK(vkCreateFramebuffer(device, &fb_gui_info, nullptr, &framebuffers[i]));
				VK_CHECK(vkCreateFramebuffer(device, &fb_offscreen_info, nullptr, &offscreenFramebuffers[i]));

				// Push all deletion functions to queue
				mainDeletionQueue.push_function([=]() {
					vkDestroyFramebuffer(device, framebuffers[i], nullptr);
					vkDestroyImageView(device, swapchainAttachments[i].imageView, nullptr);
					vkDestroyFramebuffer(device, offscreenFramebuffers[i], nullptr);
					vkDestroyImageView(device, offscreenAttachments[i].imageView, nullptr);
				});
			}
		}

		void VulkanEngine::InitSyncStructures()
		{
			// Create Syncronization Structures
			// We want to create fence with Create Signaled flag
			// so we can waut on it before using
			VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

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
			VkFenceCreateInfo uploadCreateFenceInfo = vkinit::fence_create_info();

			VK_CHECK(vkCreateFence(device, &uploadCreateFenceInfo, nullptr, &uploadContext.uploadFence));

			// enqueue destruction of upload fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(device, uploadContext.uploadFence, nullptr);
			});
		}

		void VulkanEngine::InitDescriptors()
		{
			// Initialize Descriptor Pools
			std::array<VkDescriptorPoolSize, 4> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainAttachments.size() * entityMap["Mesh"].size());
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainAttachments.size() * entityMap["Mesh"].size());
			poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[2].descriptorCount = static_cast<uint32_t>(swapchainAttachments.size() * entityMap["Mesh"].size());
			poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[3].descriptorCount = static_cast<uint32_t>(swapchainAttachments.size() * entityMap["Mesh"].size());

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = static_cast<uint32_t>(swapchainAttachments.size() * entityMap["Mesh"].size());

			VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

			// Initialize Descriptor Layouts
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding lightLayoutBinding = {};
			lightLayoutBinding.binding = 1;
			lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lightLayoutBinding.descriptorCount = 1;
			lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			lightLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding viewLayoutBinding = {};
			viewLayoutBinding.binding = 2;
			viewLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			viewLayoutBinding.descriptorCount = 1;
			viewLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			viewLayoutBinding.pImmutableSamplers = nullptr;

			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			samplerLayoutBinding.binding = 3;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, lightLayoutBinding, viewLayoutBinding, samplerLayoutBinding };

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
		}

		void VulkanEngine::InitPipelines()
		{
			// Read Shader Code from files
			auto vertShaderCode = ReadFile("shaders/vert.spv");
			auto fragShaderCode = ReadFile("shaders/frag.spv");

			// Create Shader Modules from code
			VkShaderModule vertShaderModule = vkinit::create_shader_module(device, vertShaderCode);
			VkShaderModule fragShaderModule = vkinit::create_shader_module(device, fragShaderCode);

			// Create Pipeline Layout Info
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info(descriptorSetLayout);
			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &meshMaterial.pipelineLayout));

			// Create Pipeline Builder object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = vkinit::vertex_input_state_create_info(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

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
			pipelineBuilder.rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = vkinit::multisampling_state_create_info();

			// Color Blending - Default RGBA Color Blending
			pipelineBuilder.colorBlendAttachment = vkinit::color_blend_attachment_state();

			// Depth Testing - Default
			pipelineBuilder.depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Assign Pipeline Layout to Pipeline
			pipelineBuilder.pipelineLayout = meshMaterial.pipelineLayout;

			// Build Pipeline
			meshMaterial.pipeline = pipelineBuilder.build_pipeline(device, renderPass);
		}

		void VulkanEngine::InitScene()
		{
			// Initialize Meshed
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& comp = world->GetComponent<MeshComponent>(entity);
				InitMesh(comp);
			}

			// Initialize Lights
			for (ECS::Entity entity : entityMap["Light"])
			{
				LightComponent& comp = world->GetComponent<LightComponent>(entity);
				InitLight(comp);
			}

			// Initialize Camera
			camera.position = glm::vec3(0.0f, 0.0f, 10.0f);
			camera.direction = glm::vec3(0.0f, 0.0f, -1.0f);
			camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
			camera.fov = 60.0f;
			camera.aspect = (float)windowExtent.width / (float)windowExtent.height;
			camera.zNear = 0.1f;
			camera.zFar = 100.0f;
			InitCamera(camera);
		}

		void VulkanEngine::InitMesh(MeshComponent& mesh)
		{
			// Load Texture Data
			Util::LoadImageFromFile(*this, mesh.texture_path.c_str(), mesh.texture);

			VkImageViewCreateInfo imageViewInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_UNORM, mesh.texture.image, VK_IMAGE_ASPECT_COLOR_BIT);
			VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &mesh.texture.imageView));

			// Load Mesh Data
			IO::LoadMesh(mesh);

			// Init Mesh Buffers
			InitVertexBuffer(mesh);
			InitIndexBuffer(mesh);
			InitUniformBuffer(mesh);

			mesh.material = &meshMaterial;
		}

		void VulkanEngine::InitLight(LightComponent& light)
		{
			VkDeviceSize bufferSize = sizeof(LightData);

			light.buffers.resize(swapchainAttachments.size());

			for (int i = 0; i < light.buffers.size(); i++)
			{
				light.buffers[i] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Add destruction of uniform buffer to deletion queue
				mainDeletionQueue.push_function([=]()
				{
					vmaDestroyBuffer(allocator, light.buffers[i].buffer, light.buffers[i].allocation);
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

			camera.yaw = -90.0f;
			camera.speed = 5.0f;

			// Calculate Camera View Matrix
			camera.matrices.view = glm::lookAt(camera.position, camera.position + camera.direction, camera.up);

			// Create Camera Uniform Buffers
			VkDeviceSize bufferSize = sizeof(ViewData);

			camera.buffers.resize(swapchainAttachments.size());

			for (int i = 0; i < camera.buffers.size(); i++)
			{
				camera.buffers[i] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Add destruction of uniform buffer to deletion queue
				mainDeletionQueue.push_function([=]()
				{
					vmaDestroyBuffer(allocator, camera.buffers[i].buffer, camera.buffers[i].allocation);
				});
			}
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
			mainDeletionQueue.push_function([=]()
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
			mainDeletionQueue.push_function([=]()
			{
				vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
			});

			// CLeanup Staging Buffer
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanEngine::InitUniformBuffer(MeshComponent& mesh)
		{
			VkDeviceSize bufferSize = sizeof(mesh.matrices);

			mesh.uniformBuffers.resize(swapchainAttachments.size());

			for (int i = 0; i < mesh.uniformBuffers.size(); i++)
			{
				mesh.uniformBuffers[i] = CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				// Add destruction of uniform buffer to deletion queue
				mainDeletionQueue.push_function([=]()
				{
					vmaDestroyBuffer(allocator, mesh.uniformBuffers[i].buffer, mesh.uniformBuffers[i].allocation);
				});
			}
		}

		void VulkanEngine::InitTextureSampler()
		{
			VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16;
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

		void VulkanEngine::InitDescriptorSets()
		{
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);

				std::vector<VkDescriptorSetLayout> layouts(swapchainAttachments.size(), descriptorSetLayout);
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainAttachments.size());
				allocInfo.pSetLayouts = layouts.data();

				mesh.descriptorSets.resize(swapchainAttachments.size());
				VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, mesh.descriptorSets.data()));

				for (int i = 0; i < swapchainAttachments.size(); i++)
				{
					VkDescriptorBufferInfo meshBufferInfo = {};
					meshBufferInfo.buffer = mesh.uniformBuffers[i].buffer;
					meshBufferInfo.offset = 0;
					meshBufferInfo.range = sizeof(mesh.matrices);

					LightComponent& light = world->GetComponent<LightComponent>(4);

					VkDescriptorBufferInfo lightBufferInfo = {};
					lightBufferInfo.buffer = light.buffers[i].buffer;
					lightBufferInfo.offset = 0;
					lightBufferInfo.range = sizeof(LightData);

					VkDescriptorBufferInfo viewBufferInfo = {};
					viewBufferInfo.buffer = camera.buffers[i].buffer;
					viewBufferInfo.offset = 0;
					viewBufferInfo.range = sizeof(ViewData);

					VkDescriptorImageInfo imageInfo = {};
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo.imageView = mesh.texture.imageView;
					imageInfo.sampler = textureSampler;

					std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = mesh.descriptorSets[i];
					descriptorWrites[0].dstBinding = 0;
					descriptorWrites[0].dstArrayElement = 0;
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[0].descriptorCount = 1;
					descriptorWrites[0].pBufferInfo = &meshBufferInfo;

					descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[1].dstSet = mesh.descriptorSets[i];
					descriptorWrites[1].dstBinding = 1;
					descriptorWrites[1].dstArrayElement = 0;
					descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[1].descriptorCount = 1;
					descriptorWrites[1].pBufferInfo = &lightBufferInfo;

					descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[2].dstSet = mesh.descriptorSets[i];
					descriptorWrites[2].dstBinding = 2;
					descriptorWrites[2].dstArrayElement = 0;
					descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrites[2].descriptorCount = 1;
					descriptorWrites[2].pBufferInfo = &viewBufferInfo;

					descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[3].dstSet = mesh.descriptorSets[i];
					descriptorWrites[3].dstBinding = 3;
					descriptorWrites[3].dstArrayElement = 0;
					descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[3].descriptorCount = 1;
					descriptorWrites[3].pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
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

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Update(UI::UIManager* UIManager, Input::InputManager* InputManager, float dt)
		{
			// Check if there are any 
			glfwPollEvents();

			UIManager->DrawUI(dt, InputManager);

			UpdateCamera(camera, InputManager, dt);

			DrawFrame(UIManager);
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

			camera.matrices.view = glm::lookAt(camera.position, camera.position + camera.direction, camera.up);
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::DrawFrame(UI::UIManager* UIManager)
		{
			// Draw ImGui
			ImGui::Render();

			// Wait until gpu has finished rendering last frame. Timeout of 1 second
			VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence)); // Reset fence

			// Request image from swapchain
			uint32_t swapchainImageIndex;
			VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 0, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

			// Now that we are sure commands are finished executing, reset command buffer
			VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().mainCommandBuffer, 0));

			VkCommandBuffer cmdMain = RecordMainCommandBuffers(swapchainImageIndex);
			VkCommandBuffer cmdGui = RecordGUICommandBuffers(swapchainImageIndex);

			// Prepare the submission into graphics queue
			// we will signal the _renderSemaphore, to signal that rendering has finished
			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submit.pWaitDstStageMask = &waitStage;

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmdMain;

			// Submit command buffer to queue and execute
			// GetCurrentFrame().renderFence will now block until graphics command finish executing
			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

			// After Main Renderpass has finished, perform GUI renderpass to render offscreen image and ImGui

			// Wait until scene has finished rendering
			VK_CHECK(vkWaitForFences(device, 1, &GetCurrentFrame().renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(device, 1, &GetCurrentFrame().renderFence)); // Reset fence

			// Pass Offscreen Framebuffer to Viewport Window
			UIManager->GetWindowViewport()->SetSceneTexture(offscreenAttachments[swapchainImageIndex]);

			// Prepare GUI submission into graphics queue
			// we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
			submit.waitSemaphoreCount = 1;
			submit.pWaitSemaphores = &GetCurrentFrame().presentSemaphore;
			submit.pCommandBuffers = &cmdGui;

			// Submit GUI command buffer to queue and execute
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

			// Begin Render Pass
			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Draw all Mesh objects
			DrawObjects(cmd, index);

			// Finalize Render Pass
			vkCmdEndRenderPass(cmd);

			// Finalize the command buffer (we can no longer add commands, so it can be executed)
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		VkCommandBuffer VulkanEngine::RecordGUICommandBuffers(uint32_t index)
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
			clearValue.color = { {1.0f, 1.0f, 1.0f, 1.0f} }; // Black

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

		void VulkanEngine::DrawObjects(VkCommandBuffer cmd, uint32_t index)
		{
			Material* lastMaterial = nullptr;
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& mesh = world->GetComponent<MeshComponent>(entity);
				LightComponent& light = world->GetComponent<LightComponent>(4);

				// Bind material pipeline if it does not match previous material
				if (mesh.material != lastMaterial);
				{
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
						mesh.material->pipeline);
					lastMaterial = mesh.material;
				}

				MeshMatrices matrice = {};

				matrice.model = BuildMeshTransform(world->GetComponent<TransformComponent>(entity));

				matrice.inv_model = glm::inverse(matrice.model);
				matrice.view = camera.matrices.view;
				matrice.proj = camera.matrices.perspective;
				matrice.proj[1][1] *= -1;

				mesh.matrices = matrice;

				// Map Mesh, Light and Camera Data to Uniform Buffers
				void* data;
				vmaMapMemory(allocator, mesh.uniformBuffers[index].allocation, &data);
				memcpy(data, &matrice, sizeof(matrice));
				vmaUnmapMemory(allocator, mesh.uniformBuffers[index].allocation);

				vmaMapMemory(allocator, light.buffers[index].allocation, &data);
				memcpy(data, &light.data, sizeof(LightData));
				vmaUnmapMemory(allocator, light.buffers[index].allocation);

				vmaMapMemory(allocator, camera.buffers[index].allocation, &data);
				memcpy(data, &camera.data, sizeof(ViewData));
				vmaUnmapMemory(allocator, camera.buffers[index].allocation);

				// Bind Vertices, Indices and Descriptor Sets
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
					mesh.material->pipelineLayout, 0, 1, &mesh.descriptorSets[index], 0, nullptr);

				// Draw Indexed Vertices
				vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
			}
		}

		glm::mat4 VulkanEngine::BuildMeshTransform(TransformComponent transform)
		{
			// Set Translation
			glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), (glm::vec3)transform.position);

			// Set Rotation
			model_transform = glm::rotate(model_transform, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			model_transform = glm::rotate(model_transform, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			model_transform = glm::rotate(model_transform, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

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

				mainDeletionQueue.flush();

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
			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(uploadContext.commandPool, 1);

			VkCommandBuffer cmd;
			VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

			// Begin Command buffer recording. The command buffer will be used only once, so let vulkan know that
			VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Execute Function
			function(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));

			VkSubmitInfo submit = vkinit::submit_info(&cmd);

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