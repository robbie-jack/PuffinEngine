#pragma once

//#include "imgui.h"
//#include "examples/imgui_impl_glfw.h"
//#include "examples/imgui_impl_vulkan.h"

#include "UIManager.h"
#include "InputManager.h"
#include "UIWindowViewport.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vk_mem_alloc.h"

#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Light.h"

#include "System.h"
#include "MeshComponent.h"
#include "TransformSystem.h"

#include <functional>
#include <vector>
#include <set>
#include <array>
#include <optional>
#include <fstream>

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

struct ViewportRender
{
	VkExtent2D extent = { 1024, 1024 };
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	VkRenderPass renderPass;
	std::vector<VkImage> images;
	std::vector<VmaAllocation> imageAllocations;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<VkCommandBuffer> commandbuffers;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE
//#ifdef _DEBUG
//#define IMGUI_VULKAN_DEBUG_REPORT
//#endif

namespace Puffin
{
	namespace Rendering
	{
		class VulkanRenderer : public System
		{
		public:

			void Init();
			bool Update(float dt);
			void SendMessage();
			MeshComponent* AddComponent();

			void InitComponent(int handle, std::string model_path, std::string texture_path);
			void InitComponentCube(int handle, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f));

			inline std::vector<MeshComponent>* GetComponents() { return &meshComponents; };

			inline GLFWwindow* GetWindow() { return window; };

			inline void SetUI(Puffin::UI::UIManager* UI) { uiManager = UI; };
			inline void SetUIWindowViewport(Puffin::UI::UIWindowViewport* UIWindowViewport) { uiWindowViewport = UIWindowViewport; };
			inline void SetInputManager(Puffin::Input::InputManager* InputManager) { inputManager = InputManager; };

			~VulkanRenderer();

			void InitTexture(Texture& texture, std::string texture_path);
		private:

			// Variables
			GLFWwindow* window;
			VkSurfaceKHR surface;
			VkInstance instance;

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

			// ImGui Variables
			VkRenderPass imguiRenderPass;
			std::vector<VkFramebuffer> imguiFramebuffers;
			VkCommandPool imguiCommandPool;
			std::vector<VkCommandBuffer> imguiCommandBuffers;
			VkDescriptorPool imguiDescriptorPool;

			// Viewport Rendering Variables
			ViewportRender viewportRender;
			Texture viewportTexture;

			// Semaphore/Flights for Synchronisation
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			std::vector<VkFence> imagesInFlight;
			size_t currentFrame = 0;

			// UI/Input Manager
			Puffin::UI::UIManager* uiManager;
			Puffin::UI::UIWindowViewport* uiWindowViewport;
			Puffin::Input::InputManager* inputManager;

			// Camera
			Camera camera;

			// Meshes
			Mesh chalet_mesh;
			Mesh engineer_mesh;
			Mesh cube_mesh;
			//Mesh light_cube;
			//std::vector<Mesh> meshes;
			std::vector<MeshComponent> meshComponents;

			Texture cube_texture;

			// Light
			Light light;

			VkDescriptorPool descriptorPool;

			VkSampler textureSampler;

			VkImage depthImage;
			//VkDeviceMemory depthImageMemory;
			VmaAllocation depthImageAllocation;
			VkImageView depthImageView;

			bool framebufferResized = false;

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

			// Initialise Window
			void InitWindow();
			//void SetupInput();

			//-------------------------------------------------------------------------------------

			// Initialize Vulkan
			void InitVulkan();
			void RecreateSwapChain();

			void CreateInstance();
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

			// Viewport Setup Functions
			void CreateViewportVariables();
			void CreateViewportRenderPass();
			void CreateViewportImages();
			void CreateViewportImageViews();
			void CreateViewportFramebuffers();
			void CreateViewportCommandBuffers();

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

			void DrawFrame(float delta_time);
			void UpdateUniformBuffers(uint32_t currentImage, float delta_time);
			void UpdateImguiCommandBuffers();

			glm::mat4 BuildMeshTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
			glm::mat4 BuildMeshTransform(EntityTransform transform);

			//-------------------------------------------------------------------------------------

			void Cleanup();
			void CleanupSwapChain();
			void CleanupViewport();
			void CleanupImGui();
			void CleanupMeshComponent(MeshComponent& mesh_component);
		};
	}
}