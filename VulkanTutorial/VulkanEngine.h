#pragma once

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H


// GLFW and GLM
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Vulkan Helper Classes
#include "VulkanTypes.h"
#include "VulkanInitializers.h"

#include "vk_mem_alloc.h" // Vulkan Memory Allocator
#include "vk-boostrap/VkBootstrap.h" // Vk Bootstrap

// Engine Includes
#include "ECS.h"

// Component Includes

// STL
#include <vector>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace Puffin
{
	namespace Rendering
	{
		class VulkanEngine : public ECS::System
		{
		public:

			GLFWwindow* Init();

			void Render();

			void Cleanup();

		private:

			// Variables
			VkInstance instance;						// Vulkan Library Handle
			VkDebugUtilsMessengerEXT debug_messenger;	// Vulkan Debug Output Handle
			VkPhysicalDevice chosenGPU;					// GPU chosen as default device
			VkDevice device;							// Vulkan device for commands
			VkSurfaceKHR surface;						// Vulkan window surface

			// Swapchain
			VkSwapchainKHR swapchain;
			VkFormat swapchainImageFormat; // Image format expected by windowing system
			std::vector<Types::FrameBufferAttachment> swapchainAttachments; // Images/Views from swapchain

			// GLFW
			GLFWwindow* window;							// Window
			GLFWmonitor* monitor;						// Monitor

			bool framebufferResized = false; // Flag to indicate if GLFW window has been resized
			bool isInitialized;

			Types::WindowExtent windowExtent;
			const int WIDTH = 1280; // Starting Window Width
			const int HEIGHT = 720; // Starting Window Height

			// Functions
			void InitVulkan();
			void InitSwapchain();

			static inline void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
				app->framebufferResized = true;
			}

		};
	}
}

#endif // VULKAN_ENGINE_H