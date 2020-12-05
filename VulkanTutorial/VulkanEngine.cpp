#include "VulkanEngine.h"

#include "VulkanInitializers.h"
#include "ModelLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb-master\stb_image.h>

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
		GLFWwindow* VulkanEngine::Init()
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

			windowExtent.width = WIDTH;
			windowExtent.height = HEIGHT;

			window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Puffin Engine", monitor, nullptr);

			//glfwMaximizeWindow(window);
			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

			// Load Core Vulkan Structures
			InitVulkan();

			// Create Swapchain
			InitSwapchain();

			// Initialize Command Pool and buffer
			InitCommands();

			// Initialise Default Renderpass
			InitDefaultRenderpass();

			// Initialise Swapchain Framebuffers
			InitFramebuffers();

			// Initialise Semaphores and Fences
			InitSyncStructures();

			// Initialize Descriptor Sets
			InitDescriptors();

			// Initialize Pipelines
			InitPipelines();

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

			// Select GPU with VK Bootstrap
			// We want a gpu which can write to glfw surface and supports vulkan 1.2
			vkb::PhysicalDeviceSelector selector{ vkb_inst };
			vkb::PhysicalDevice physicalDevice = selector
				.set_minimum_version(1, 2)
				.set_surface(surface)
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
				FrameBufferAttachment swapchainAttachment;
				swapchainAttachment.image = vkbSwapchain.get_images().value()[i];
				swapchainAttachment.imageView = vkbSwapchain.get_image_views().value()[i];
				swapchainAttachments.push_back(swapchainAttachment);
			}

			swapchainImageFormat = vkbSwapchain.image_format;

			mainDeletionQueue.push_function([=]()
			{
				vkDestroySwapchainKHR(device, swapchain, nullptr);
			});

			// Depth image size will match window
			VkExtent3D depthImageExtent =
			{
				windowExtent.width,
				windowExtent.height,
				1
			};

			// Hardcode depth format to 32 bit float
			depthFormat = VK_FORMAT_D32_SFLOAT;

			// Depth image will use format we selected and depth attachment usage flag
			VkImageCreateInfo depthImageInfo = VKInit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

			// Allocate depth image from local gpu memory
			VmaAllocationCreateInfo depthImageAllocInfo = {};
			depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Allocate and create image
			vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthAttachment.image, &depthAttachment.allocation, nullptr);

			// Build Image View for depth image to use in rendering
			VkImageViewCreateInfo depthImageViewInfo = VKInit::imageview_create_info(depthFormat, depthAttachment.image, VK_IMAGE_ASPECT_DEPTH_BIT);

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
			VkCommandPoolCreateInfo commandPoolInfo = VKInit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

			// Allocate Default Command Buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = VKInit::command_buffer_allocate_info(commandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));

			mainDeletionQueue.push_function([=]() {
				vkDestroyCommandPool(device, commandPool, nullptr);
			});

			// Create Upload Command Pool
			VkCommandPoolCreateInfo uploadCommandPoolInfo = VKInit::command_pool_create_info(graphicsQueueFamily);

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
			colorAttachment.format = swapchainImageFormat; // Will want to replace this with the offscreen image format later
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

		void VulkanEngine::InitFramebuffers()
		{
			// Create Framebuffers for swapchain images.
			// This will connect render pass to images for rendering
			VkFramebufferCreateInfo fb_info = {};
			fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_info.pNext = nullptr;

			fb_info.renderPass = renderPass;
			fb_info.width = windowExtent.width;
			fb_info.height = windowExtent.height;
			fb_info.layers = 1;

			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();
			framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

			// Create Framebuffers for each of teh swapchain image views
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				VkImageView attachments[2];
				attachments[0] = swapchainAttachments[i].imageView;
				attachments[1] = depthAttachment.imageView;

				fb_info.pAttachments = attachments;
				fb_info.attachmentCount = 2;

				VK_CHECK(vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffers[i]));

				mainDeletionQueue.push_function([=]() {
					vkDestroyFramebuffer(device, framebuffers[i], nullptr);
					vkDestroyImageView(device, swapchainAttachments[i].imageView, nullptr);
				});
			}
		}

		void VulkanEngine::InitSyncStructures()
		{
			// Create Syncronization Structures
			// We want to create fence with Create Signaled flag
			// so we can waut on it before using
			VkFenceCreateInfo fenceCreateInfo = VKInit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

			VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

			//enqueue the destruction of the fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(device, renderFence, nullptr);
			});

			// Create Upload Fence
			VkFenceCreateInfo uploadCreateFenceInfo = VKInit::fence_create_info();

			VK_CHECK(vkCreateFence(device, &uploadCreateFenceInfo, nullptr, &uploadContext.uploadFence));

			// enqueue destruction of upload fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(device, uploadContext.uploadFence, nullptr);
			});

			// Semaphores don't need any flags
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;

			VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
			VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));

			//enqueue the destruction of semaphores
			mainDeletionQueue.push_function([=]() {
				vkDestroySemaphore(device, presentSemaphore, nullptr);
				vkDestroySemaphore(device, renderSemaphore, nullptr);
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
			VkShaderModule vertShaderModule = VKInit::create_shader_module(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::create_shader_module(device, fragShaderCode);

			// Create Pipeline Layout Info
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::pipeline_layout_create_info(descriptorSetLayout);
			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &material.pipelineLayout));

			// Create Pipeline Builder object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage Info
			pipelineBuilder.shaderStages.push_back(VKInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(VKInit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			auto bindingDescription = Vertex::getBindingDescription();
			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::vertex_input_state_create_info(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = VKInit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Define Viewport
			pipelineBuilder.viewport.x = 0.0f;
			pipelineBuilder.viewport.y = 0.0f;
			pipelineBuilder.viewport.width = (float)windowExtent.width;
			pipelineBuilder.viewport.height = (float)windowExtent.height;
			pipelineBuilder.viewport.minDepth = 0.0f;
			pipelineBuilder.viewport.maxDepth = 1.0f;

			// Define Scissor Extent (Pixels Outside Scissor Rectangle will be discarded)
			pipelineBuilder.scissor.offset = { 0, 0 };
			pipelineBuilder.scissor.extent = windowExtent;

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = VKInit::multisampling_state_create_info();

			// Color Blending - Default RGBA Color Blending
			pipelineBuilder.colorBlendAttachment = VKInit::color_blend_attachment_state();

			// Depth Testing - Default
			pipelineBuilder.depthStencil = VKInit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

			// Assign Pipeline Layout to Pipeline
			pipelineBuilder.pipelineLayout = material.pipelineLayout;

			// Build Pipeline
			material.pipeline = pipelineBuilder.build_pipeline(device, renderPass);
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
			camera.position = glm::vec3(0.0f, 0.0f, 1.0f);
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
			//InitTexture(comp.texture, comp.texture_path);

			// Load Mesh Data
			IO::LoadMesh(mesh);

			// Copy Loaded Mesh data into mesh vertex buffer
			const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);

			// Allocate Staging Buffer - Map Vertices in CPU Memory
			AllocatedBuffer stagingBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

			// Map vertex data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Allocate Vertex Buffer - Transfer Vertices into GPU optimised Memory
			mesh.vertexBuffer = CreateBuffer(bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy;
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = bufferSize;
				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);
			});

			// Add destruction of mesh buffer to deletion queue
			mainDeletionQueue.push_function([=]()
			{
				vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
			});

			// Cleanup Staging Buffer Immediately, It is no longer needed
			vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
		}

		void VulkanEngine::InitLight(LightComponent& light)
		{

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
		}

		void VulkanEngine::InitTextureImage(Texture& texture, std::string texture_path)
		{
			int texWidth, texHeight, texChannels;

			// Load Texture from file
			stbi_uc* pixels = stbi_load(texture_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			// Get required image size for texture
			VkDeviceSize imageSize = texWidth * texHeight * 4;

			if (!pixels)
			{
				throw std::runtime_error("failed to load texture image!");
			}

			// VMA Buffer Creation
			/*CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingAllocation);*/

			// Create Staging Buffer/Memory for image
			AllocatedBuffer stagingBuffer = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

			// Copy Texture data to staging buffer
			void* data;
			vmaMapMemory(allocator, stagingBuffer.allocation, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vmaUnmapMemory(allocator, stagingBuffer.allocation);

			// Free Loaded texture
			stbi_image_free(pixels);

			/*CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				texture.image, VMA_MEMORY_USAGE_GPU_ONLY, texture.allocation);*/

			VkExtent3D extent
			{
				texWidth,
				texHeight,
				0
			};

			// Create Initial Image
			VkImageCreateInfo imageInfo = VKInit::image_create_info(
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &texture.image, &texture.allocation, nullptr))
			{
				throw std::runtime_error("failed to create image!");
			}

			/*TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);*/

			/*CopyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));*/

			/*TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);*/

			vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
			vmaFreeMemory(allocator, stagingBuffer.allocation);
		}

		AllocatedBuffer VulkanEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
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

			AllocatedBuffer newBuffer;
			
			// Allocate Buffer
			VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));

			return newBuffer;
		}

		void VulkanEngine::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
		{

		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Render()
		{
			// Wait until gpu has finished rendering last frame. Timeout of 1 second
			VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 1000000000)); // Wait for fence to complete
			VK_CHECK(vkResetFences(device, 1, &renderFence)); // Reset fence

			// Request image from swapchain
			uint32_t swapchainImageIndex;
			VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 0, presentSemaphore, nullptr, &swapchainImageIndex));

			// Now that we are sure commands are finished executing, reset command buffer
			VK_CHECK(vkResetCommandBuffer(mainCommandBuffer, 0));

			VkCommandBuffer cmd = RecordMainCommandBuffers(swapchainImageIndex);

			// Prepare the submission into graphics queue
			// we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
			// we will signal the _renderSemaphore, to signal that rendering has finished

			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			submit.pWaitDstStageMask = &waitStage;

			submit.waitSemaphoreCount = 1;
			submit.pWaitSemaphores = &presentSemaphore;

			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &renderSemaphore;

			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmd;

			// Submit command buffer to queue and execute
			// renderFence will now block until graphics command finish executing
			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence));

			// This will present the image we just rendered to the visible window
			// we want to wait on renderSemaphore for that
			// as its necessary that drawing commands have finished before the image is displayed to the user
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;

			presentInfo.pSwapchains = &swapchain;
			presentInfo.swapchainCount = 1;

			presentInfo.pWaitSemaphores = &renderSemaphore;
			presentInfo.waitSemaphoreCount = 1;

			presentInfo.pImageIndices = &swapchainImageIndex;

			VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));
		}

		VkCommandBuffer VulkanEngine::RecordMainCommandBuffers(uint32_t index)
		{
			// Name buffer cmd for shorter writing
			VkCommandBuffer cmd = mainCommandBuffer;

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
			//float flash = abs(sin(frameNumber / 120.0f));
			clearValue.color = { {0.4f, 0.70f, 1.0f, 1.0f} };

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
			rpInfo.renderArea.extent = windowExtent;
			rpInfo.framebuffer = framebuffers[index];

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

		void VulkanEngine::DrawObjects(VkCommandBuffer cmd, uint32_t index)
		{
			for (ECS::Entity entity : entityMap["Mesh"])
			{
				MeshComponent& comp = world->GetComponent<MeshComponent>(entity);
				LightComponent& light = world->GetComponent<LightComponent>(4);

				MeshMatrices matrice = {};

				matrice.model = BuildMeshTransform(world->GetComponent<TransformComponent>(entity));

				matrice.inv_model = glm::inverse(matrice.model);
				matrice.view = camera.matrices.view;
				matrice.proj = camera.matrices.perspective;
				matrice.proj[1][1] *= -1;

				comp.matrices = matrice;

				void* data;
				vmaMapMemory(allocator, comp.uniformAllocations[index], &data);
				memcpy(data, &matrice, sizeof(matrice));
				vmaUnmapMemory(allocator, comp.uniformAllocations[index]);

				vmaMapMemory(allocator, light.allocations[index], &data);
				memcpy(data, &light.uniformBuffer, sizeof(LightBufferObject));
				vmaUnmapMemory(allocator, light.allocations[index]);

				vmaMapMemory(allocator, camera.viewAllocations[index], &data);
				memcpy(data, &camera.viewBufferObject, sizeof(ViewBufferObject));
				vmaUnmapMemory(allocator, camera.viewAllocations[index]);
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
				vkWaitForFences(device, 1, &renderFence, true, 1000000000);

				mainDeletionQueue.flush();

				//// Destroy Command Pool
				//vkDestroyCommandPool(device, commandPool, nullptr);

				//// Destroy Swapchain
				//vkDestroySwapchainKHR(device, swapchain, nullptr);

				//// Destroy Renderpass
				//vkDestroyRenderPass(device, renderPass, nullptr);

				//// Destory Swapchain Resoruces
				//for (int i = 0; i < swapchainAttachments.size(); i++)
				//{
				//	vkDestroyFramebuffer(device, framebuffers[i], nullptr);
				//	vkDestroyImageView(device, swapchainAttachments[i].imageView, nullptr);
				//}

				vkDestroyDevice(device, nullptr);
				vkDestroySurfaceKHR(instance, surface, nullptr);
				//vkb::destroy_debug_utils_messenger(instance, debug_messenger);
				vkDestroyInstance(instance, nullptr);
			}
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
		{
			// Allocate Default command buffer that will be used for instant commands
			VkCommandBufferAllocateInfo cmdAllocInfo = VKInit::command_buffer_allocate_info(uploadContext.commandPool, 1);

			VkCommandBuffer cmd;
			VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

			// Begin Command buffer recording. The command buffer will be used only once, so let vulkan know that
			VkCommandBufferBeginInfo cmdBeginInfo = VKInit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

			// Execute Function
			function(cmd);

			VK_CHECK(vkEndCommandBuffer(cmd));

			VkSubmitInfo submit = VKInit::submit_info(&cmd);

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