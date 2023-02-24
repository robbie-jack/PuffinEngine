#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>

#include <vulkan/vulkan.hpp>

//#include "vku/vku_framework.hpp"
#include "vku/vku.hpp"
#include "vk_mem_alloc.hpp"

#include "ECS/System.hpp"
#include "Types/DeletionQueue.hpp"
#include "Types/Vertex.hpp"
#include "VKTypes.hpp"
#include "VKDescriptors.hpp"
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
		vk::DescriptorSetLayout instanceSetLayout;
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
		
		// Material Data (Set for each unique material i.e textures)
		vk::DescriptorSet materialDescriptor;

		// Instance Data (Set for each instance)
		vk::DescriptorSet instanceDescriptor;
		
	};

	constexpr uint32_t G_BUFFERED_FRAMES = 2;

	constexpr uint32_t G_MAX_OBJECTS = 10000;

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
		vk::SwapchainKHR m_swapchain;
		vk::Format m_swapchainImageFormat;
		std::vector<vk::Image> m_swapchainImages;
		std::vector<vk::ImageView> m_swapchainImageViews;
		AllocatedImage m_swapchainDepthImage;

		// Command Execution
		vk::Queue m_graphicsQueue;
		uint32_t m_graphicsQueueFamily;

		vk::RenderPass m_renderPass;
		std::vector<vk::Framebuffer> m_framebuffers;

		StaticRenderData m_staticRenderData;
		std::array<FrameRenderData, G_BUFFERED_FRAMES> m_frameRenderData;

		PackedVector<MeshData> m_meshData;
		std::unordered_map<UUID, std::set<ECS::EntityID>> m_meshDrawList;

		uint32_t m_frameNumber;

		// Pipelines
		vk::PipelineCache m_pipelineCache;

		vku::ShaderModule m_triVertMod, m_triFragMod;
		vk::UniquePipelineLayout m_triPipelineLayout;
		vk::UniquePipeline m_triPipeline;

		vku::ShaderModule m_forwardVertMod, m_forwardFragMod;
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

		void InitVulkan();
		void InitSwapchain();
		void InitCommands();
		void InitDefaultRenderPass();
		void InitFramebuffers();
		void InitSyncStructures();
		void InitBuffers();
		void InitDescriptors();
		void InitPipelines();

		void BuildTrianglePipeline();
		void BuildForwardRendererPipeline();

		void ProcessEvents();
		void ProcessComponents();
		void UpdateEditorCamera();
		void UpdateRenderData();
		void Draw();

		void UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity);

		void PrepareSceneData();
		void DrawObjects(vk::CommandBuffer cmd);

		glm::mat4 BuildModelTransform(const Vector3f& position, const Vector3f& rotation, const Vector3f& scale);

		bool LoadMesh(UUID meshID, MeshData& meshData);
		void UnloadMesh(MeshData& meshData) const;

		FrameRenderData& GetCurrentFrameData()
		{
			return m_frameRenderData[m_frameNumber % G_BUFFERED_FRAMES];
		}
	};
}
