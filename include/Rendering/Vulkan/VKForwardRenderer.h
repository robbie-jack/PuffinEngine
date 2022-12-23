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

		VKForwardRenderer(std::shared_ptr<VulkanRenderSystem> vulkanRenderSystem): VKBaseRenderer(vulkanRenderSystem) {}
		~VKForwardRenderer() override = default;

		void Setup() override;

		VkSemaphore& DrawScene(const int& frameIndex) override;

		void Cleanup() override;

	private:

		std::vector<ForwardFrameData> m_forwardFrameData;

		VkExtent3D m_bufferExtent = { 0, 0, 0 };

		VkPipeline m_pipeline;
		VkPipelineLayout m_pipelineLayout;

		void SetupCommandBuffers();
		void SetupSemaphores();
		void SetupPipeline();

		VkCommandBuffer RecordCommandBuffer(const int& frameIndex);

	};
}
