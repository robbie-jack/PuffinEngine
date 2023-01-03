#pragma once

// GLFW and GLM
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Vulkan Helper Classes
#include <Rendering/Vulkan/VKTypes.h>
#include <Rendering/Vulkan/Helpers/VKPipeline.h>
#include <Rendering/Vulkan/DebugDraw.h>

#include "Rendering/Vulkan/VKForwardRenderer.h"
#include <Rendering/Vulkan/VKDeferredRender.h>

//#include "vk_mem_alloc.h" // Vulkan Memory Allocator
#include <vk-bootstrap/VkBootstrap.h> // Vk Bootstrap

// Engine Includes
#include <ECS/ECS.h>
#include <UI/Editor/UIManager.h>
#include <Input/InputSubsystem.h>

// Component Includes
#include <Components/TransformComponent.h>
#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>
#include <Components/Rendering/CameraComponent.h>

// Type Includes
#include "Types/RingBuffer.h"
#include "Helpers/VKVertex.hpp"

// STL
#include <vector>
#include <unordered_map>
#include <set>
#include <fstream>
#include <memory>

#include "Components/Rendering/CameraComponent.h"
#include "Components/Rendering/CameraComponent.h"

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
		const int MAX_INSTANCES = 10000;
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
			VkSemaphore presentSemaphore, renderSemaphore, shadowmapSemaphore;
			VkFence renderFence;

			VkCommandPool commandPool; // Command Pool for our commands
			VkCommandBuffer mainCommandBuffer, guiCommandBuffer, shadowCommandBuffer; // Buffer commands are recorded into

			AllocatedBuffer cameraViewProjBuffer;
			VkDescriptorSet cameraViewProjDescriptor;

			AllocatedBuffer objectBuffer;
			AllocatedBuffer instanceBuffer;
			VkDescriptorSet objectInstanceDescriptor;

			AllocatedBuffer lightSpaceBuffer;
			VkDescriptorSet lightSpaceDescriptor;

			VkDescriptorSet matTextureDescriptor;

			AllocatedBuffer cameraShadingBuffer; // Buffer Containing Camera/Debug data for Fragment Shader
			VkDescriptorSet cameraShadingDescriptor;

			AllocatedBuffer lightBuffer, pointLightBuffer, dirLightBuffer, spotLightBuffer, lightStatsBuffer;
			VkDescriptorSet lightDataDescriptor;

			// Shadowmap Rendering
			VkDescriptorSet shadowmapDescriptor;
			std::unordered_map<ECS::EntityID, AllocatedImage> shadowmapImages;
			std::unordered_map<ECS::EntityID, VkFramebuffer> shadowmapFramebuffers;

			// Scene Data
			IndirectDrawBatch drawBatch;

			// Debug Variables
			std::vector<VertexPC32> debugVertices;
			std::vector<uint32_t> debugIndices;
			std::vector<VkDrawIndexedIndirectCommand> debugIndirectCommands;
			AllocatedBuffer debugVertexBuffer, debugIndexBuffer, debugIndirectCommandsBuffer;
		};

		struct SwapchainData
		{
			AllocatedImage attachment;
			VkFramebuffer framebuffer;

			bool needsRecreated = false;
		};

		struct OffscreenData
		{
			AllocatedImage attachment;
			AllocatedImage depthAttachment;
			VkFramebuffer framebuffer;
			ImTextureID viewportTexture;

			bool needsRecreated = false;
		};

		const Vector2f boxPositions[4] =
		{
			Vector2f(-0.5f, -0.5f),	// Bottom Left
			Vector2f(-0.5f, 0.5f),	// Top Left
			Vector2f(0.5f, 0.5f),	// Top Right
			Vector2f(0.5f, -0.5f)	// Bottom Right
		};

		const uint32_t boxIndices[8] =
		{
			0, 1,
			1, 2,
			2, 3,
			3, 0
		};

		const Vector3f cubePositions[8] =
		{
			Vector3f(-1.0f, 1.0f, 1.0f),
			Vector3f(-1.0f, -1.0f, 1.0f),
			Vector3f(1.0f, -1.0f, 1.0f),
			Vector3f(1.0f, 1.0f, 1.0f),
			Vector3f(-1.0f, 1.0f, -1.0f),
			Vector3f(-1.0f, -1.0f, -1.0f),
			Vector3f(1.0f, -1.0f, -1.0f),
			Vector3f(1.0f, 1.0f, -1.0f),
		};

		const uint32_t cubeIndices[24] =
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

		class VulkanRenderSystem : public ECS::System, public std::enable_shared_from_this<VulkanRenderSystem>
		{
		public:

			VulkanRenderSystem()
			{
				m_systemInfo.name = "VulkanRenderSystem";
				m_systemInfo.updateOrder = Core::UpdateOrder::Render;
			}

			~VulkanRenderSystem() override
			{
				m_uiManager = nullptr;
				m_inputSubsystem = nullptr;

				m_inputEvents->Flush();
				m_inputEvents = nullptr;

				m_drawLineEvents->Flush();
				m_drawLineEvents = nullptr;

				m_drawBoxEvents->Flush();
				m_drawBoxEvents = nullptr;
			}

			// Main Functions
			void Init() override;
			void PreStart() override;
			void Start() override {}
			void Update() override;
			void Stop()  override;
			void Cleanup() override;

			// Helper Functions
			void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
			AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlags requiredFlags = 0);

			// Memory allocator
			VmaAllocator m_allocator;
			UploadContext uploadContext;
			DeletionQueue mainDeletionQueue;

		private:

			friend VKForwardRenderer;

			std::shared_ptr<UI::UIManager> m_uiManager;
			std::shared_ptr<Input::InputSubsystem> m_inputSubsystem;

			// Variables
			VkInstance instance;						// Vulkan Library Handle
			VkDebugUtilsMessengerEXT debug_messenger;	// Vulkan Debug Output Handle
			VkPhysicalDevice physicalDevice;			// Handle to physical GPU chosen for rendering
			VkDevice m_device;							// Vulkan device for commands
			VkSurfaceKHR surface;						// Vulkan window surface

			// Swapchain
			VkSwapchainKHR m_swapchain = VK_NULL_HANDLE, m_oldSwapchain = VK_NULL_HANDLE;
			VkFormat swapchainImageFormat; // Image format expected by windowing system

			vkb::Swapchain m_vkbSwapchain;
			std::vector<SwapchainData> m_swapchainData;

			// Offscreen
			VkExtent2D m_offscreenExtent;
			VkFormat offscreenFormat;
			ImVec2 m_viewportSize; // Size of ImGui Viewport

			std::vector<OffscreenData> m_offscreenData;

			bool m_shadowmapsNeedsUpdated;

			// Shadows
			VkFormat shadowFormat;
			VkPipelineLayout shadowPipelineLayout;
			VkPipeline shadowPipeline;

			// Forward Renderer
			std::shared_ptr<VKForwardRenderer> m_forwardRenderer = nullptr;

			// Deferred Renderer
			VKDeferredRender deferredRenderer;

			// Debug Pipeline
			VkPipelineLayout debugPipelineLayout;
			VkPipeline debugPipeline;

			FrameData m_frames[FRAME_OVERLAP];

			SceneRenderData m_sceneRenderData;

			// Depth Resources
			VkFormat depthFormat;

			// Descriptor Abstractions
			VKUtil::DescriptorAllocator* descriptorAllocator;
			VKUtil::DescriptorLayoutCache* descriptorLayoutCache;

			VkRenderPass m_renderPass;
			VkRenderPass renderPassGUI;
			VkRenderPass renderPassShadows;

			// Pipelines/Materials
			//Material meshMaterial;

			//VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout m_cameraViewProjSetLayout;
			VkDescriptorSetLayout m_objectInstanceSetLayout;
			VkDescriptorSetLayout m_matTextureSetLayout;
			VkDescriptorSetLayout m_cameraShadingSetLayout;
			VkDescriptorSetLayout m_lightDataSetLayout;
			VkDescriptorSetLayout m_shadowMapSetLayout;
			VkDescriptorSetLayout m_lightSpaceSetLayout;

			VkQueue graphicsQueue; // queue we will submit to
			uint32_t graphicsQueueFamily; // family of that queue

			VkSampler textureSampler;
			VkSampler depthSampler; // Sampler for reading shadowmaps in shader

			// Camera
			EditorCamera editorCamera;
			bool moveForward = false;
			bool moveBackward = false;
			bool moveLeft = false;
			bool moveRight = false;
			bool moveUp = false;
			bool moveDown = false;

			// GLFW
			GLFWwindow* window;							// Window
			GLFWmonitor* monitor;						// Monitor

			bool m_framebufferResized = false; // Flag to indicate if GLFW window has been resized
			bool isInitialized = false;
			bool m_needsStarted = true; // Scene Rendering data needs setup in PreStart function

			VkExtent2D windowExtent;
			int frameNumber = 0;

			// Event Buffers
			std::shared_ptr<RingBuffer<Input::InputEvent>> m_inputEvents;
			std::shared_ptr<RingBuffer<Debug::Line>> m_drawLineEvents;
			std::shared_ptr<RingBuffer<Debug::Box>> m_drawBoxEvents;

			// Init Main Methods
			void InitVulkan();
			void InitSwapchain();
			void UpdateSwapchainImageViews();
			void UpdateSwapchainImageView(uint32_t index);
			void InitOffscreenAttachments();
			void InitOffscreenAttachment(uint32_t index);
			void InitOffscreenDepthAttachment(uint32_t index);
			void InitCommands();

			// Init Renderpass Methods
			void InitDefaultRenderpass();
			void InitGUIRenderpass();
			void InitShadowRenderPass();

			// Init Framebuffer Methods
			void InitSwapchainFramebuffers();
			void InitSwapchainFramebuffer(uint32_t index);
			void InitOffscreenFramebuffers();
			void InitOffscreenFramebuffer(uint32_t index);
			void InitSyncStructures();

			// Init Buffer Methods
			void InitBuffers();
			void InitSceneBuffers();

			// Init Descriptor Methods
			void InitDescriptors();
			void InitCameraVPDescriptors();
			void InitObjectInstanceDescriptors();
			void InitCameraShadingDescriptors();
			void InitLightDataDescriptors();
			void InitLightSpaceDescriptors();

			// Init Pipeline Methods
			void InitShadowPipeline();
			void InitDebugPipeline();

			// Init Scene Methods
			void InitScene();
			void InitMatTextureDescriptors();
			void InitShadowmapDescriptors();

			// Update Scene Methods
			void UpdateShadowmapDescriptors();

			// Init ImGui Methods
			void InitImGui();
			void InitImguiTextures();
			void InitImGuiTexture(uint32_t index);
			void DestroyImguiTexture(uint32_t index);

			// Init Sampler Methods
			void InitTextureSampler();
			void InitDepthSampler();

			// Init Renderer Methods
			void SetupDeferredRenderer();

			// Functions for Initializing/Destroying Swapchain and Offscreen Variables
			void UpdatSwapchainData(uint32_t index);
			void DestroySwapchainData(uint32_t index);

			void UpdateOffscreenData(uint32_t index);
			void DestroyOffscreenData(uint32_t index);

			// Init Component Functions
			void InitMesh(ECS::EntityID entityID, UUID meshID);
			void InitProceduralMesh(ECS::EntityID entity);
			void InitShadowcasterLight(ECS::EntityID entity);
			void InitCamera(ECS::EntityID entity);

			void InitEditorCamera();

			void InitAlbedoTexture(UUID uuid);

			// Init Buffer Functions
			AllocatedBuffer InitVertexBuffer(const void* vertexData, const size_t verticesSize);
			AllocatedBuffer InitIndexBuffer(const void* indexData, const size_t indicesSize);

			void CopyVerticesToBuffer(const void* vertexData, const size_t verticesSize, AllocatedBuffer vertexBuffer, uint32_t copyOffset = 0);
			void CopyIndicesToBuffer(const void* indexData, const size_t indicesSize, AllocatedBuffer indexBuffer, uint32_t copyOffset = 0);

			// Component Cleanup Functions
			void CleanupTexture(ECS::EntityID entityID, UUID textureID);
			void CleanupMesh(ECS::EntityID entityID, UUID meshID);
			void CleanupShadowcasterLight(ECS::EntityID entity);

			// Update Functions
			void ProcessEvents();
			void UpdateCamera(ECS::EntityID entity);
			void UpdateEditorCamera();

			// Render Functions
			void DrawFrame();

			/*
			* Prepare Scene/Data Data for rendering
			*/
			void PrepareScene();
			void UpdateMergedVertexBuffer(const uint32_t newSize);
			void UpdateMergedIndexBuffer(const uint32_t newSize);
			void AddMeshRenderDataToScene(MeshRenderData& meshRenderData, const MeshBufferData& meshBufferData);

			void PrepareLights();

			glm::mat4 CalculateLightSpaceView(const float& aspectRatio, const float& outerRadius, const Vector3f& position, const Vector3f& direction) const;

			VkCommandBuffer RecordShadowCommandBuffers(uint32_t index);
			void RecordShadowRenderPass(VkCommandBuffer cmd, const VkRenderPassBeginInfo& renderPassInfo, const glm::mat4& lightSpaceView);

			VkCommandBuffer RecordMainCommandBuffers(uint32_t index);
			VkCommandBuffer RecordGUICommandBuffer(uint32_t index);

			void DrawDebugObjects(VkCommandBuffer cmd, uint32_t index);

			void MapObjectData();

			glm::mat4 BuildMeshTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale) const;

			void CleanupBuffers();

			// Debug Draw Functions
			void DrawDebugLine(Debug::Line line);
			void DrawDebugBox(Debug::Box box);
			void DrawDebugCube(Debug::Cube cube);

			static inline void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<VulkanRenderSystem*>(glfwGetWindowUserPointer(window));
				app->m_framebufferResized = true;
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
				return m_frames[frameNumber % FRAME_OVERLAP];
			}
		};
	}
}