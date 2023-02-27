#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.hpp"

#include "ECS/System.hpp"
#include "Types/DeletionQueue.hpp"
#include "Types/Vertex.hpp"
#include "VKTypes.hpp"
#include "VKDescriptors.hpp"
#include "VKPipeline.hpp"
#include "Assets/TextureAsset.h"
#include "ECS/Entity.h"
#include "Components/Rendering/CameraComponent.h"
#include "Input/InputEvent.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace Puffin::Rendering::VK
{
	// Struct containing render data that is static between frames
	struct StaticRenderData
	{
		std::shared_ptr<Util::DescriptorAllocator> descriptorAllocator = nullptr;
		std::shared_ptr<Util::DescriptorLayoutCache> descriptorLayoutCache = nullptr;

		vk::DescriptorSetLayout globalSetLayout;
		vk::DescriptorSetLayout materialSetLayout;

		vk::Sampler textureSampler;
	};

	// Struct containing data that changes each frame
	struct FrameRenderData
	{
		// Synchronization
		vk::Semaphore presentSemaphore, renderSemaphore;
		vk::Fence renderFence;

		// Command Execution
		vk::CommandPool commandPool;
		vk::CommandBuffer mainCommandBuffer;

		// Global Data (Set for entire frame)
		vk::DescriptorSet globalDescriptor;

		AllocatedBuffer cameraBuffer;
		AllocatedBuffer objectBuffer;
		AllocatedBuffer lightBuffer;
		AllocatedBuffer lightStaticBuffer;
		
		// Material Data (Set for each unique material i.e textures)
		vk::DescriptorSet materialDescriptor;

		bool swapchainNeedsUpdated = false;
		bool textureDescriptorNeedsupdated = false;
	};

	const static std::unordered_map<Assets::TextureFormat, vk::Format> g_texFormatMap =
	{
		{ Assets::TextureFormat::RGBA8, vk::Format::eR8G8B8A8Unorm }
	};

	constexpr uint32_t G_BUFFERED_FRAMES = 2;

	constexpr uint32_t G_MAX_OBJECTS = 10000;
	constexpr uint32_t G_MAX_LIGHTS = 8;

	// Vulkan Rendering System
	class VKRenderSystem : public ECS::System, public std::enable_shared_from_this<VKRenderSystem>
	{
	public:

		VKRenderSystem()
		{
			m_systemInfo.name = "VKRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override;
		void Stop() override {}
		void Cleanup() override;

		const vma::Allocator& GetAllocator() const { return m_allocator ;}
		const vk::Device& GetDevice() const { return m_device; }
		const UploadContext& GetUploadContext() const { return m_uploadContext; }
		const vk::Queue& GetGraphicsQueue() const { return m_graphicsQueue; }

		void OnInputEvent(const Input::InputEvent& inputEvent);

	private:

		// Initialization Members
		vk::Device m_device;
		vk::Instance m_instance;
		vk::PhysicalDevice m_physicalDevice;
		vk::SurfaceKHR m_surface;
		vk::DebugUtilsMessengerEXT m_debugMessenger;

		vk::Extent2D m_windowSize;

		vma::Allocator m_allocator;

		// Swapchain
		SwapchainData m_swapchainData;
		SwapchainData m_oldSwapchainData;

		// Command Execution
		vk::Queue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;

		vk::RenderPass m_renderPass;

		StaticRenderData m_staticRenderData;
		std::array<FrameRenderData, G_BUFFERED_FRAMES> m_frameRenderData;

		PackedVector<MeshData> m_meshData;
		std::unordered_map<UUID, std::set<ECS::EntityID>> m_meshDrawList;

		PackedVector<TextureData> m_texData;
		std::unordered_map<UUID, std::set<ECS::EntityID>> m_texDrawList;

		uint32_t m_frameNumber;

		// Pipelines
		vk::PipelineCache m_pipelineCache;

		Util::ShaderModule m_forwardVertMod, m_forwardFragMod;
		vk::UniquePipelineLayout m_forwardPipelineLayout;
		vk::UniquePipeline m_forwardPipeline;

		UploadContext m_uploadContext;

		DeletionQueue m_deletionQueue;

		EditorCamera m_editorCam;
		CameraMatComponent m_editorCamMats;
		bool m_moveLeft = false;
		bool m_moveRight = false;
		bool m_moveForward = false;
		bool m_moveBackward = false;
		bool m_moveUp = false;
		bool m_moveDown = false;

		RingBuffer<Input::InputEvent> m_inputEvents;

		// Indicated initialization completed without any failures
		bool m_isInitialized = false;
		bool m_windowResized = false;

		void InitVulkan();
		void InitSwapchain(SwapchainData& swapchainData, vk::SwapchainKHR& oldSwapchain);
		void InitCommands();
		void InitDefaultRenderPass();
		void InitSwapchainFramebuffers(SwapchainData& swapchainData);
		void InitSyncStructures();
		void InitBuffers();
		void InitSamplers();
		void InitDescriptors();
		void InitPipelines();

		void BuildForwardRendererPipeline();

		void ProcessEvents();
		void ProcessComponents();
		void UpdateEditorCamera();
		void UpdateRenderData();
		void Draw();

		void UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity);

		void RecreateSwapchain();
		void CleanSwapchain(SwapchainData& swapchainData);

		void UpdateTextureDescriptors();

		void PrepareSceneData();

		vk::CommandBuffer RecordMainCommandBuffer(uint32_t swapchainIdx);

		void DrawObjects(vk::CommandBuffer cmd);

		void SubmitCommands(uint32_t swapchainIdx, std::vector<vk::CommandBuffer>& commands);

		glm::mat4 BuildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale);

		bool LoadMesh(UUID meshID, MeshData& meshData);
		void UnloadMesh(MeshData& meshData) const;

		bool LoadTexture(UUID texID, TextureData& texData);
		void UnloadTexture(TextureData& texData) const;

		void BuildTextureDescriptorInfo(PackedVector<TextureData>& texData, std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;

		FrameRenderData& GetCurrentFrameData()
		{
			return m_frameRenderData[m_frameNumber % G_BUFFERED_FRAMES];
		}

		static inline void FrameBufferResizeCallback(GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<VKRenderSystem*>(glfwGetWindowUserPointer(window));

			app->m_windowResized = true;
			app->m_windowSize.width = width;
			app->m_windowSize.height = height;
		}
	};
}
