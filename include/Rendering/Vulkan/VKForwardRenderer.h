#pragma once

#include "VKBaseRenderer.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace Puffin::Rendering
{
	// Forward Declaration
	struct ForwardFrameData
	{
		// Command Buffer
		VkCommandBuffer commandBuffer;

		// Synchronisation
		VkSemaphore renderSemaphore;
	};

	class VKForwardRenderer : public VKBaseRenderer
	{
	public:

		VKForwardRenderer(): VKBaseRenderer() {}
		~VKForwardRenderer() override = default;

		void Setup(std::shared_ptr<VulkanRenderSystem> vulkanRenderSystem) override;

		VkSemaphore& DrawScene(const int& frameIndex, const int& swapchainIndex) override;

		void Cleanup() override;

		void SetRenderExtent(const VkExtent2D renderExtent)
		{
			m_renderExtent = { renderExtent.width, renderExtent.height, 1 };
		}

	private:

		std::vector<ForwardFrameData> m_forwardFrameData;

		VkExtent3D m_renderExtent = { 0, 0, 0 };

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;

		void SetupCommandBuffers();
		void SetupSemaphores();
		void SetupPipeline();

		VkCommandBuffer RecordCommandBuffer(const int& frameIndex, const int& swapchainIndex);

	};
}
