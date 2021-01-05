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
#include "VKTypes.h"
#include "VKPipeline.h"

//#include "vk_mem_alloc.h" // Vulkan Memory Allocator
#include "vk-boostrap/VkBootstrap.h" // Vk Bootstrap

// Engine Includes
#include "ECS.h"
#include "UIManager.h"
#include "InputManager.h"

// Component Includes
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "CameraComponent.h"

// STL
#include <vector>
#include <deque>
#include <unordered_map>
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
		namespace VKUtil
		{
			class DescriptorLayoutCache;
			class DescriptorAllocator;
		}

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

		/*struct RenderObject
		{
			MeshComponent& mesh;

			Material* material;

			glm::mat4 transformMatrix;
		};*/

		struct UploadContext
		{
			VkFence uploadFence;
			VkCommandPool commandPool;
		};

		struct LightStatsData
		{
			int numPLights;
			int numDLights;
			int numSLights;
		};

		struct FrameData
		{
			VkSemaphore presentSemaphore, renderSemaphore;
			VkFence renderFence;

			VkCommandPool commandPool, guiCommandPool, shadowCommandPool; // Command Pool for our commands
			VkCommandBuffer mainCommandBuffer, guiCommandBuffer, shadowCommandBuffer; // Buffer commands are recorded into

			AllocatedBuffer cameraViewProjBuffer, cameraBuffer;
			VkDescriptorSet cameraViewProjDescriptor, cameraDescriptor;

			AllocatedBuffer pointLightBuffer, directionalLightBuffer, spotLightBuffer, lightStatsBuffer;
			VkDescriptorSet lightDescriptor;

			AllocatedBuffer objectBuffer;
			VkDescriptorSet objectDescriptor;

			AllocatedBuffer lightSpaceBuffer, lightSpaceMultiBuffer, lightSpaceIndexBuffer;
			VkDescriptorSet lightSpaceDescriptor, lightSpaceMultiDescriptor;

			VkDescriptorSet shadowmapDescriptor;
		};

		struct GPUObjectData
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 inv_model;
		};

		struct GPUCameraData
		{
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
		};

		struct GPULightSpaceData
		{
			alignas(16) glm::mat4 lightSpaceMatrix;
		};

		struct GPULightIndexData
		{
			alignas(4) int lightSpaceIndex;
		};

		// Number of frames to overlap when rendering
		constexpr unsigned int FRAME_OVERLAP = 2;
		const int WIDTH = 1280; // Starting Window Width
		const int HEIGHT = 720; // Starting Window Height

		const int MAX_LIGHTS = 4;

		class VulkanEngine : public ECS::System
		{
		public:

			// Main Functions
			GLFWwindow* Init(UI::UIManager* UIManager);

			bool Update(UI::UIManager* UIManager, Input::InputManager* InputManager, float dt);

			void Cleanup();

			// Helper Functions
			void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
			AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags = 0);

			// Memory allocator
			VmaAllocator allocator;
			UploadContext uploadContext;
			DeletionQueue mainDeletionQueue;
			DeletionQueue swapchainDeletionQueue;
			DeletionQueue offscreenDeletionQueue;

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
			std::vector<AllocatedImage> swapchainAttachments; // Images/Views from swapchain
			std::vector<VkFramebuffer> framebuffers;

			// Offscreen
			VkExtent2D offscreenExtent;
			VkFormat offscreenFormat;
			std::vector<AllocatedImage> offscreenAttachments; // Images/Views for Offscreen Rendering
			std::vector<VkFramebuffer> offscreenFramebuffers;
			std::vector<ImTextureID> viewportTextureIDs; // Vector of Texture ID's which are passed to Viewport Draw function
			ImVec2 viewportSize; // Size of ImGui Viewport
			bool offscreenInitialized;

			std::vector<ImTextureID> shadowTextureIDs;

			// Shadows
			VkExtent2D shadowExtent; // Resolution of rendered shadowmaps
			VkFormat shadowFormat;
			VkPipelineLayout shadowPipelineLayout;
			VkPipeline shadowPipeline;
			

			FrameData frames[FRAME_OVERLAP];

			// Depth Resources
			AllocatedImage depthAttachment;
			VkFormat depthFormat;

			// Descriptor Abstractions
			VKUtil::DescriptorAllocator* descriptorAllocator;
			VKUtil::DescriptorLayoutCache* descriptorLayoutCache;

			VkRenderPass renderPass;
			VkRenderPass renderPassGUI;
			VkRenderPass renderPassShadows;

			// Pipelines/Materials
			Material meshMaterial;

			//VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout cameraViewProjSetLayout;
			VkDescriptorSetLayout objectSetLayout;

			VkDescriptorSetLayout cameraSetLayout;
			VkDescriptorSetLayout lightSetLayout;
			VkDescriptorSetLayout shadowMapSetLayout;
			VkDescriptorSetLayout singleTextureSetLayout;

			VkDescriptorSetLayout lightSpaceSetLayout;
			VkDescriptorSetLayout lightSpaceMultiSetLayout;

			VkQueue graphicsQueue; // queue we will submit to
			uint32_t graphicsQueueFamily; // family of that queue

			VkSampler textureSampler;
			VkSampler depthSampler; // Sampler for reading shadowmaps in shader

			//std::vector<RenderObject> renderObjects;
			//std::unordered_map<std::string_view, Material> materials;

			// Camera
			CameraComponent camera;

			// GLFW
			GLFWwindow* window;							// Window
			GLFWmonitor* monitor;						// Monitor

			bool framebufferResized = false; // Flag to indicate if GLFW window has been resized
			bool isInitialized;

			VkExtent2D windowExtent;
			int frameNumber = 0;
			float prevfov;

			// Init Main Functions
			void InitVulkan();
			void InitSwapchain();
			void InitOffscreen();
			void InitCommands();
			void InitDefaultRenderpass();
			void InitGUIRenderpass();
			void InitShadowRenderPass();
			void InitFramebuffers();
			void InitOffscreenFramebuffers();
			void InitSyncStructures();
			void InitDescriptors();
			void InitPipelines();
			void InitShadowPipeline();
			void InitScene();
			void InitShadowmapDescriptors();
			void InitImGui();
			void InitImGuiTextureIDs();
			void InitShadowTextureIDs();
			void InitTextureSampler();
			void InitDepthSampler();

			// Functions for Re-Initializing Swapchain and Offscreen Variables
			void RecreateSwapchain();
			void RecreateOffscreen();

			// Init Component Functions
			void InitMesh(MeshComponent& mesh);
			void InitLight(LightComponent& light);
			void InitCamera(CameraComponent& camera);

			// Init Buffer Functions
			void InitVertexBuffer(MeshComponent& mesh);
			void InitIndexBuffer(MeshComponent& mesh);

			// Update Functions
			void UpdateCamera(CameraComponent& camera, Puffin::Input::InputManager* inputManager, float delta_time);

			// Render Functions
			void DrawFrame(UI::UIManager* UIManager);
			VkCommandBuffer RecordShadowCommandBuffers(uint32_t index);
			VkCommandBuffer RecordMainCommandBuffers(uint32_t index);
			VkCommandBuffer RecordGUICommandBuffer(uint32_t index);
			//void RenderShadowPass(VkCommandBuffer cmd, uint32_t index);
			void DrawObjects(VkCommandBuffer cmd, uint32_t index);
			void MapObjectData();
			glm::mat4 BuildMeshTransform(TransformComponent comp);

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

			FrameData& GetCurrentFrame()
			{
				return frames[frameNumber % FRAME_OVERLAP];
			}
		};
	}
}

#endif // VULKAN_ENGINE_H