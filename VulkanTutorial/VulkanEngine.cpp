#include "VulkanEngine.h"

#include "VulkanInitializers.h"

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
				Types::FrameBufferAttachment swapchainAttachment;
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
		}

		void VulkanEngine::InitDefaultRenderpass()
		{
			// Renderpass will use this color attachment
			VkAttachmentDescription color_attachment = {};
			// attachment will have the format needed by the swapchain
			color_attachment.format = swapchainImageFormat; // Will want to replace this with the offscreen image format later
			// 1 sample, we won't be doing msaa
			color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			// we clear when this attachment is loaded
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			// keep the attachment stored when renderpass ends
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			// don't care about stencil
			color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			// don't know or care about started layout of the attachment
			color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// after renderpass ends, the imahe has to be on a layout ready for display
			color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachment_ref = {};
			// attachment number will index into pAttachments array in parent renderpass itself
			color_attachment_ref.attachment = 0;
			color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// We are goin to create 1 subpass, which is the minimum you can do
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment_ref;

			// Create Renderpass
			VkRenderPassCreateInfo render_pass_info = {};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			// connect color attachment to info
			render_pass_info.attachmentCount = 1;
			render_pass_info.pAttachments = &color_attachment;
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
			fb_info.attachmentCount = 1;
			fb_info.width = windowExtent.width;
			fb_info.height = windowExtent.height;
			fb_info.layers = 1;

			// Grab how many images we have in swapchain
			const uint32_t swapchain_imagecount = swapchainAttachments.size();
			framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

			// Create Framebuffers for each of teh swapchain image views
			for (int i = 0; i < swapchain_imagecount; i++)
			{
				fb_info.pAttachments = &swapchainAttachments[i].imageView;
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

			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.pNext = nullptr;

			// We want to create fence with Create Signaled flag
			// so we can waut on it before using
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

			//enqueue the destruction of the fence
			mainDeletionQueue.push_function([=]() {
				vkDestroyFence(device, renderFence, nullptr);
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

			frameNumber++;
		}

		VkCommandBuffer VulkanEngine::RecordMainCommandBuffers(uint32_t swapchainImageIndex)
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
			VkClearValue clearValue;
			float flash = abs(sin(frameNumber / 120.0f));
			clearValue.color = { {0.0f, 0.0f, flash, 1.0f} };

			// Start Main Renderpass
			//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
			VkRenderPassBeginInfo rpInfo = {};
			rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpInfo.pNext = nullptr;

			rpInfo.renderPass = renderPass;
			rpInfo.renderArea.offset.x = 0;
			rpInfo.renderArea.offset.y = 0;
			rpInfo.renderArea.extent = windowExtent;
			rpInfo.framebuffer = framebuffers[swapchainImageIndex];

			// Connect clear values
			rpInfo.clearValueCount = 1;
			rpInfo.pClearValues = &clearValue;

			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Finalize Render Pass
			vkCmdEndRenderPass(cmd);

			// Finalize the command buffer (we can no longer add commands, so it can be executed)
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
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
	}
}