#include "VulkanEngine.h"

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

			glfwMaximizeWindow(window);
			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

			// Load Core Vulkan Structures
			InitVulkan();

			// Create Swapchain
			InitSwapchain();

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
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Render()
		{
			
		}

		//-------------------------------------------------------------------------------------

		void VulkanEngine::Cleanup()
		{
			if (isInitialized)
			{
				// Destroy Swapchain
				vkDestroySwapchainKHR(device, swapchain, nullptr);

				// Destory Swapchain Resoruces
				for (int i = 0; i < swapchainAttachments.size(); i++)
				{
					vkDestroyImageView(device, swapchainAttachments[i].imageView, nullptr);
				}

				vkDestroyDevice(device, nullptr);
				vkDestroySurfaceKHR(instance, surface, nullptr);
				vkb::destroy_debug_utils_messenger(instance, debug_messenger);
				vkDestroyInstance(instance, nullptr);
			}
		}
	}
}