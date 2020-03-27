#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "InputManager.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Light.h"

#include "System.h"
#include "RenderComponent.h"

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

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//#define IMGUI_UNLIMITED_FRAME_RATE
//#ifdef _DEBUG
//#define IMGUI_VULKAN_DEBUG_REPORT
//#endif

class VulkanRenderer : public System
{
public:

	void Init();
	bool Update(float dt);
	void SendMessage();
	RenderComponent* AddComponent();

	void InitComponent(int handle, std::string model_path, std::string texture_path);
	void InitComponentCube(int handle, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f));

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

	// Semaphore/Flights for Synchronisation
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;

	// Input Manager
	InputManager inputManager;

	// Camera
	Camera camera;

	// Meshes
	Mesh chalet_mesh;
	Mesh engineer_mesh;
	Mesh cube_mesh;
	//Mesh light_cube;
	//std::vector<Mesh> meshes;
	std::vector<RenderComponent> render_components;

	Texture cube_texture;

	// Light
	Light light;

	VkDescriptorPool descriptorPool;

	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	bool framebufferResized = false;

	float fps = 60.0f;
	float fps_timer = 0.0f;

	const int WIDTH = 1920;
	const int HEIGHT = 1080;

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
	void SetupInput();

	//-------------------------------------------------------------------------------------

	// Initialize Vulkan
	void InitVulkan();
	void RecreateSwapChain();

	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateCommandPool(VkCommandPool& command_pool, VkCommandPoolCreateFlags flags);
	void CreateDepthResources();
	void CreateFrameBuffers();
	void CreateImGuiFramebuffers();

	void CreateTextureImage(Texture& texture, std::string texture_path);
	void CreateTextureImageView(Texture& texture);
	void CreateTextureSampler();

	void CreateVertexBuffers(RenderComponent& render_component);
	void CreateIndexBuffers(RenderComponent& render_component);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateUniformBuffer(RenderComponent& render_component);
	void CreateUniformBuffers();
	void CreateLightBuffers();
	void CreateViewBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateMainCommandBuffers();
	void CreateImGuiCommandBuffers();
	void CreateCommandBuffers(VkCommandBuffer* command_buffer, uint32_t command_buffer_count, VkCommandPool &command_pool);
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

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void SetupImGui();
	void SetupImGuiWindow();
	void CreateImGuiDescriptorPool();
	void CreateImGuiRenderPass();
	VkSurfaceFormatKHR SelectSurfaceFormats(const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space);
	VkPresentModeKHR SelectPresentMode(const VkPresentModeKHR* request_modes, int request_modes_count);

	//-------------------------------------------------------------------------------------

	//void MainLoop();
	//void Update(float delta_time);
	void DrawFrame(float delta_time);
	void DrawUI(float delta_time);
	void UpdateUniformBuffers(uint32_t currentImage, float delta_time);
	glm::mat4 BuildMeshTransform(Transform transform);
	glm::mat4 BuildMeshTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
	void UpdateImguiCommandBuffers();

	//-------------------------------------------------------------------------------------

	void Cleanup();
	void CleanupSwapChain();
	void CleanupImGui();
	void CleanupRenderComponent(RenderComponent& render_component);
};