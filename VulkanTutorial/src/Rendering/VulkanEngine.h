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
#include <Rendering/VKTypes.h>
#include <Rendering/VKPipeline.h>
#include <Rendering/DebugDraw.h>

#include <Rendering/VKDeferredRender.h>

//#include "vk_mem_alloc.h" // Vulkan Memory Allocator
#include <vk-boostrap/VkBootstrap.h> // Vk Bootstrap

// Engine Includes
#include <ECS/ECS.h>
#include <UI/UIManager.h>
#include <Input/InputManager.h>

// Component Includes
#include <Components/TransformComponent.h>
#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>
#include <Components/Rendering/CameraComponent.h>

// Type Includes
#include <Types/RingBuffer.h>

// STL
#include <vector>
#include <unordered_map>
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
		// Number of frames to overlap when rendering
		constexpr unsigned int FRAME_OVERLAP = 2;
		const int WIDTH = 1280; // Starting Window Width
		const int HEIGHT = 720; // Starting Window Height

		const int MAX_LIGHTS_PER_TYPE = 4;
		const int MAX_OBJECTS = 10000;
		const int MAX_TEXTURES = 10000;
		const int MAX_DEBUG_COMMANDS = 10000;
		const int MAX_VERTICES_PER_COMMAND = 8;
		const int MAX_INDICES_PER_COMMAND = 24;

		namespace VKUtil
		{
			class DescriptorLayoutCache;
			class DescriptorAllocator;
		}

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

		struct FrameData
		{
			VkSemaphore presentSemaphore, renderSemaphore;
			VkFence renderFence;

			VkCommandPool commandPool; // Command Pool for our commands
			VkCommandBuffer mainCommandBuffer, guiCommandBuffer, shadowCommandBuffer; // Buffer commands are recorded into

			AllocatedBuffer cameraViewProjBuffer, cameraBuffer;
			VkDescriptorSet cameraViewProjDescriptor, cameraDescriptor;

			VkDescriptorSet lightDescriptor;

			AllocatedBuffer objectBuffer;
			VkDescriptorSet objectDescriptor;

			AllocatedBuffer lightSpaceBuffer, lightSpaceMultiBuffer, lightSpaceIndexBuffer;
			VkDescriptorSet lightSpaceDescriptor, lightSpaceMultiDescriptor;

			VkDescriptorSet shadowmapDescriptor;

			// Scene Data
			VkDescriptorSet geometryDescriptor;
			IndirectDrawBatch drawBatch;

			AllocatedBuffer uboBuffer; // Buffer Containing Camera/Debug data for Fragment Shader
			AllocatedBuffer pointLightBuffer, dirLightBuffer, spotLightBuffer, lightStatsBuffer;

			// Debug Variables
			std::vector<Vertex> debugVertices;
			std::vector<uint32_t> debugIndices;
			std::vector<VkDrawIndexedIndirectCommand> debugIndirectCommands;
			AllocatedBuffer debugVertexBuffer, debugIndexBuffer, debugIndirectCommandsBuffer;
		};

		struct GPUObjectData
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 inv_model;
		};

		struct GPUCameraData
		{
			alignas(16) glm::mat4 viewProj;
		};

		struct GPULightSpaceData
		{
			alignas(16) glm::mat4 lightSpaceMatrix;
		};

		struct GPULightIndexData
		{
			alignas(4) int lightSpaceIndex;
		};

		const Vector3 boxPositions[8] =
		{
			Vector3(-1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, 1.0f),
			Vector3(1.0f, -1.0f, 1.0f),
			Vector3(1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, 1.0f, -1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f),
			Vector3(1.0f, 1.0f, -1.0f),
		};

		const uint32_t boxIndices[24] =
		{
			0, 1, // Front Lines
			1, 2,
			2, 3,
			3, 0,
			4, 5, // Back Lines
			5, 6,
			6, 7,
			7, 4,
			0, 4, // Side Lines
			1, 5,
			2, 6,
			3, 7
		};

		class VulkanEngine : public ECS::System
		{
		public:

			// Main Functions
			GLFWwindow* Init(GLFWwindow* windowIn, UI::UIManager* inUIManager, Input::InputManager* inInputManager);
			void Init() override;
			void PreStart() override;
			void Start() override;
			void Update() override;
			void Stop()  override;
			void Cleanup() override;

			ECS::SystemInfo GetInfo() override
			{
				ECS::SystemInfo info;

				info.updateOrder = ECS::UpdateOrder::None;

				return info;
			}

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

			UI::UIManager* m_uiManager;
			Input::InputManager* m_inputManager;

			// Variables
			VkInstance instance;						// Vulkan Library Handle
			VkDebugUtilsMessengerEXT debug_messenger;	// Vulkan Debug Output Handle
			VkPhysicalDevice physicalDevice;			// Handle to physical GPU chosen for rendering
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
			bool shadowmapDescriptorNeedsUpdated;

			// Shadows
			VkExtent2D shadowExtent; // Resolution of rendered shadowmaps
			VkFormat shadowFormat;
			VkPipelineLayout shadowPipelineLayout;
			VkPipeline shadowPipeline;

			// Deferred Renderer
			VKDeferredRender deferredRenderer;

			// Debug Pipeline
			VkPipelineLayout debugPipelineLayout;
			VkPipeline debugPipeline;

			FrameData frames[FRAME_OVERLAP];

			SceneData sceneData;

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

			VkDescriptorSetLayout geometrySetLayout; // Descriptor Layout for Deferred Geometry Pass

			VkQueue graphicsQueue; // queue we will submit to
			uint32_t graphicsQueueFamily; // family of that queue

			VkSampler textureSampler;
			VkSampler depthSampler; // Sampler for reading shadowmaps in shader

			// Camera
			CameraComponent camera;
			bool moveForward, moveBackward, moveLeft, moveRight, moveUp, moveDown;

			// GLFW
			GLFWwindow* window;							// Window
			GLFWmonitor* monitor;						// Monitor

			bool framebufferResized = false; // Flag to indicate if GLFW window has been resized
			bool isInitialized;

			VkExtent2D windowExtent;
			int frameNumber = 0;
			float prevfov;

			int CURRENT_VERTEX_BUFFER_SIZE = 750000;
			int CURRENT_INDEX_BUFFER_SIZE = 300000;

			// Event Buffers
			std::shared_ptr<RingBuffer<Input::InputEvent>> inputEvents;
			std::shared_ptr<RingBuffer<Debug::Line>> drawLineEvents;
			std::shared_ptr<RingBuffer<Debug::Box>> drawBoxEvents;

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
			void InitBuffers();
			void InitSceneBuffers();
			void InitDescriptors();
			void InitDeferredDescriptors();
			void InitPipelines();
			void InitShadowPipeline();
			void InitDebugPipeline();
			void InitScene();
			void InitShadowmapDescriptors();
			void InitImGui();
			void InitImGuiTextureIDs();
			void InitTextureSampler();
			void InitDepthSampler();

			void SetupDeferredRenderer();

			// Functions for Re-Initializing Swapchain and Offscreen Variables
			void RecreateSwapchain();
			void RecreateOffscreen();

			void StartScene();

			// Init Component Functions
			void InitMesh(MeshComponent& mesh);
			void InitLight(LightComponent& light);
			void InitCamera(CameraComponent& camera);

			// Init Buffer Functions
			AllocatedBuffer InitVertexBuffer(const std::vector<Vertex>& vertices);
			AllocatedBuffer InitIndexBuffer(const std::vector<uint32_t>& indices);

			void CopyVerticesToBuffer(const std::vector<Vertex>& vertices, AllocatedBuffer vertexBuffer, uint32_t copyOffset = 0);
			void CopyIndicesToBuffer(const std::vector<uint32_t>& indices, AllocatedBuffer indexBuffer, uint32_t copyOffset = 0);

			// Component Cleanup Functions
			void CleanupMesh(MeshComponent& mesh);
			void CleanupLight(LightComponent& light);

			// Update Functions
			void ProcessEvents();
			void UpdateCamera(CameraComponent& camera);

			// Render Functions
			void DrawFrame();

			/*
			* Prepare Scene/Data Data for rendering
			*/
			void PrepareScene();
			void PrepareLights();

			VkCommandBuffer RecordShadowCommandBuffers(uint32_t index);
			VkCommandBuffer RecordMainCommandBuffers(uint32_t index);
			VkCommandBuffer RecordGUICommandBuffer(uint32_t index);

			void DrawObjects(VkCommandBuffer cmd, uint32_t index);
			void DrawDebugObjects(VkCommandBuffer cmd, uint32_t index);

			void MapObjectData();
			glm::mat4 BuildMeshTransform(TransformComponent comp);

			// Debug Draw Functions
			void DrawDebugLine(Debug::Line line);
			void DrawDebugBox(Debug::Box box);

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