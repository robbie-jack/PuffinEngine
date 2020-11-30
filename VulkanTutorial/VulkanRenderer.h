#pragma once

#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include "ECS.h"
#include "UIManager.h"
#include "InputManager.h"
#include "UIWindowViewport.h"
#include "UIWindowSettings.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk_mem_alloc.h"

//#include "Mesh.h"
//#include "Texture.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "FrameBufferAttachment.h"

#include <functional>
#include <vector>
#include <set>
#include <array>
#include <optional>
#include <fstream>
#include <iostream>

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Structs
struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//const bool enableValidationLayers = false;

//#define IMGUI_UNLIMITED_FRAME_RATE
//#ifdef _DEBUG
//#define IMGUI_VULKAN_DEBUG_REPORT
//#endif

namespace Puffin
{
	namespace Rendering
	{
		class VulkanRenderer : public ECS::System
		{
		public:

			void Init(UI::UIManager* UIManager, Input::InputManager* InputManager);
			bool Update(UI::UIManager* UIManager, Input::InputManager* InputManager, float dt);
			void Cleanup();

			void InitMesh(ECS::Entity entity, std::string model_path, std::string texture_path);
			void InitMeshCube(ECS::Entity entity, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f));
			void InitLight(LightComponent& light, glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, float specular = 0.5f, int shininess = 32);
			void InitCamera(CameraComponent& camera, glm::vec3 position_, glm::vec3 direction_, glm::vec3 up_, float fov, float aspect, float near, float far);

			inline GLFWwindow* GetWindow() { return window; };

			~VulkanRenderer();

			void InitTexture(Texture& texture, std::string texture_path);
		private:

			// Variables
			GLFWwindow* window;
			GLFWmonitor* monitor;
			VkSurfaceKHR surface;
			VkInstance instance;
			VkDebugUtilsMessengerEXT debugMessenger;

			int minImageCount;

			// Physical/Logical Device
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice device;

			// Memory Allocator
			VmaAllocator allocator;

			// Queues
			VkQueue graphicsQueue;
			VkQueue presentQueue;

			// Swap Chain
			VkSwapchainKHR swapChain;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			std::vector<VkImage> swapChainImages;
			std::vector<VkImageView> swapChainImageViews;
			std::vector<VkFramebuffer> swapChainFramebuffers;

			// Graphics Pipeline
			VkRenderPass renderPass;
			VkDescriptorSetLayout descriptorSetLayout;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;

			VkPipelineCache pipelineCache;

			// Command Pool/Buffers
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;

			// Offscreen Render Variables
			Texture offscreenTexture;
			VkExtent2D offscreenExtent;
			VkFormat offscreenFormat;
			std::vector<VkFramebuffer> offscreenFramebuffers;
			std::vector<FrameBufferAttachment> offscreenAttachments;
			FrameBufferAttachment offscreenDepthAttachment;

			// ImGui Variables
			VkRenderPass imguiRenderPass;
			std::vector<VkFramebuffer> imguiFramebuffers;
			VkCommandPool imguiCommandPool;
			std::vector<VkCommandBuffer> imguiCommandBuffers;
			VkDescriptorPool imguiDescriptorPool;

			// Semaphore/Flights for Synchronization
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			std::vector<VkFence> imagesInFlight;
			size_t currentFrame = 0;

			// Camera
			CameraComponent camera;

			Texture cube_texture;

			// Light
			LightComponent light;

			VkDescriptorPool descriptorPool;

			VkSampler textureSampler;

			FrameBufferAttachment depthAttachment;

			bool framebufferResized = false;

			bool recreateSwapChain = false;

			const int WIDTH = 1280;
			const int HEIGHT = 720;

			const int MAX_FRAMES_IN_FLIGHT = 2;

			// Functions

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

			static inline void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
				app->framebufferResized = true;
			}

			static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
				auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
				if (func != nullptr) {
					return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
				}
				else {
					return VK_ERROR_EXTENSION_NOT_PRESENT;
				}
			}

			static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
				auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
				if (func != nullptr) {
					func(instance, debugMessenger, pAllocator);
				}
			}

			static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData) 
			{

				std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

				return VK_FALSE;
			}

			void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
				createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				createInfo.pfnUserCallback = DebugCallback;
			}

			// Initialize Window
			void InitWindow(Input::InputManager* InputManager);

			//-------------------------------------------------------------------------------------

			// Initialize Vulkan Renderer
			void InitVulkan(UI::UIManager* UIManager);
			void RecreateSwapChain(UI::UIManager* UIManager);

			void CreateInstance();
			void SetupDebugMessenger();
			void CreateSurface();
			void PickPhysicalDevice();
			void CreateLogicalDevice();
			void CreateAllocator();
			void CreateSwapChain();
			void CreateImageViews();
			void CreateRenderPass();
			void CreateDescriptorSetLayout();
			void CreateGraphicsPipeline();
			void CreateCommandPool(VkCommandPool& command_pool, VkCommandPoolCreateFlags flags);
			void CreateDepthResources();
			void CreateFrameBuffers();

			void CreateTextureImage(Texture& texture, std::string texture_path);
			void CreateTextureImageView(Texture& texture);
			void CreateTextureSampler();

			void CreateVertexBuffers(MeshComponent& mesh_component);
			void CreateIndexBuffers(MeshComponent& mesh_component);

			void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

			void CreateUniformBuffer(MeshComponent& mesh_component);
			void CreateUniformBuffers();
			void CreateLightBuffers();
			void CreateViewBuffers();
			void CreateDescriptorPool();
			void CreateDescriptorSets();
			void CreateMainCommandBuffers();
			
			void CreateCommandBuffers(VkCommandBuffer* command_buffer, uint32_t command_buffer_count, VkCommandPool& command_pool);
			void CreateSyncObjects();

			bool CheckValidationLayerSupport();
			std::vector<const char*> GetRequiredExtensions();
			int RateDeviceSuitability(VkPhysicalDevice device);
			bool IsDeviceSuitable(VkPhysicalDevice device);
			bool CheckDeviceExtensionsSupport(VkPhysicalDevice device);
			SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
			QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

			VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

			VkFormat FindDepthFormat();
			VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

			VkShaderModule CreateShaderModule(const std::vector<char>& code);

			void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaMemoryUsage allocationUsage, VmaAllocation& allocation);
			VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

			uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaMemoryUsage allocationUsage, VmaAllocation& allocation);
			void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
			void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

			VkCommandBuffer BeginSingleTimeCommands();
			void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

			// Offscreen Functions
			void InitOffscreen();
			void CreateOffscreenVariables();
			void CreateOffscreenAttachments();
			void CreateOffscreenDepthAttachment();
			void CreateOffscreenFramebuffers();

			// ImGui Functions
			void SetupImGui();
			void SetupImGuiWindow();

			void CreateImGuiVariables();
			void CreateImGuiRenderPass();
			void CreateImGuiFramebuffers();
			void CreateImGuiDescriptorPool();
			void CreateImGuiCommandBuffers();

			VkSurfaceFormatKHR SelectSurfaceFormats(const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
			VkPresentModeKHR SelectPresentMode(const VkPresentModeKHR* request_modes, int request_modes_count);

			//-------------------------------------------------------------------------------------

			void DrawFrame(UI::UIManager* UIManager, float delta_time);
			void UpdateUniformBuffers(uint32_t currentImage, float delta_time);
			void UpdateImguiCommandBuffers(uint32_t currentImage);

			void UpdateCamera(CameraComponent& camera, Puffin::Input::InputManager* inputManager, float delta_time);

			glm::mat4 BuildMeshTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
			glm::mat4 BuildMeshTransform(TransformComponent transform);

			//-------------------------------------------------------------------------------------

			void CleanupSwapChain();
			void CleanupOffscreen();
			void CleanupImGui();
			void CleanupMeshComponent(MeshComponent& mesh_component);
			void CleanupFrameBufferAttachment(FrameBufferAttachment& attachment);
		};
	}
}

#endif // VULKAN_RENDERER_H