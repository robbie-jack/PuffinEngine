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
#include "VulkanPipeline.h"

#include "vk_mem_alloc.h" // Vulkan Memory Allocator
#include "vk-boostrap/VkBootstrap.h" // Vk Bootstrap

// Engine Includes
#include "ECS.h"

// Component Includes
#include "MeshComponent.h"

// STL
#include <vector>
#include <deque>
#include <functional>
#include <fstream>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace Puffin
{
	namespace Rendering
	{
		struct DeletionQueue
		{
			std::deque<std::function<void()>> deletors;

			void push_function(std::function<void()>&& function)
			{
				deletors.push_back(function);
			}

			void flush()
			{
				// reverse iterate the deletion queue to execute all the functions
				for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
					(*it)(); //call functors
				}

				deletors.clear();
			}
		};

		struct Material
		{
			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
		};

		struct RenderObject
		{
			MeshComponent& mesh;

			Material* material;

			glm::mat4 transformMatrix;
		};

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

			// Memory allocator
			VmaAllocator allocator;

			// Swapchain
			VkSwapchainKHR swapchain;
			VkFormat swapchainImageFormat; // Image format expected by windowing system
			std::vector<Types::FrameBufferAttachment> swapchainAttachments; // Images/Views from swapchain
			std::vector<VkFramebuffer> framebuffers;

			// Depth Resources
			FrameBufferAttachment depthAttachment;
			VkFormat depthFormat;

			VkRenderPass renderPass;
			VkPipeline graphicsPipeline;
			VkPipelineLayout pipelineLayout;

			VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout descriptorSetLayout;

			VkQueue graphicsQueue; // queue we will submit to
			uint32_t graphicsQueueFamily; // family of that queue

			VkCommandPool commandPool; // Command Pool for our commands
			VkCommandBuffer mainCommandBuffer; // Buffer commands are recorded into

			VkSemaphore presentSemaphore, renderSemaphore;
			VkFence renderFence;

			// GLFW
			GLFWwindow* window;							// Window
			GLFWmonitor* monitor;						// Monitor

			DeletionQueue mainDeletionQueue;

			bool framebufferResized = false; // Flag to indicate if GLFW window has been resized
			bool isInitialized;

			VkExtent2D windowExtent;
			const int WIDTH = 1280; // Starting Window Width
			const int HEIGHT = 720; // Starting Window Height
			int frameNumber = 0;

			// Init Functions
			void InitVulkan();
			void InitSwapchain();
			void InitCommands();
			void InitDefaultRenderpass();
			void InitFramebuffers();
			void InitSyncStructures();
			void InitDescriptors();
			void InitPipelines();

			// Render Functions
			VkCommandBuffer RecordMainCommandBuffers(uint32_t swapchainImageIndex);

			static inline void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
				app->framebufferResized = true;
			}

			static inline std::vector<char> ReadFile(const std::string& filename)
			{
				std::ifstream file(filename, std::ios::ate | std::ios::binary);

				if (!file.is_open())
				{
					throw std::runtime_error("failed to open file!");
				}

				size_t fileSize = (rsize_t)file.tellg();
				std::vector<char> buffer(fileSize);

				file.seekg(0);
				file.read(buffer.data(), fileSize);

				file.close();

				//std::cout << "BufferSize: " << buffer.size() << std::endl;

				return buffer;
			}

		};
	}
}

#endif // VULKAN_ENGINE_H