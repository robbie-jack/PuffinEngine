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
#include "ECS/Entity.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace Puffin::Rendering::VK
{
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

		vk::CommandPool m_commandPool;
		vk::CommandBuffer m_mainCommandBuffer;

		vk::RenderPass m_renderPass;
		std::vector<vk::Framebuffer> m_framebuffers;

		// Synchronization
		vk::Semaphore m_presentSemaphore, m_renderSemaphore;
		vk::Fence m_renderFence;

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

		PackedVector<MeshData> m_meshData;

		DeletionQueue m_deletionQueue;

		// Indicated initialization completed without any failures
		bool m_isInitialized = false;

		void InitVulkan();
		void InitSwapchain();
		void InitCommands();
		void InitDefaultRenderPass();
		void InitFramebuffers();
		void InitSyncStructures();
		void InitPipelines();

		void BuildTrianglePipeline();
		void BuildForwardRendererPipeline();

		void InitComponents();
		void UpdateComponents();
		void CleanupComponents();

		void Draw();

		void InitMeshComponent(std::shared_ptr<ECS::Entity> entity);
		void CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity);
	};
}
