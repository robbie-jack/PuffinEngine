#include "Rendering/Vulkan/VKForwardRenderer.h"

#include "Assets/AssetRegistry.h"
#include "Rendering/Vulkan/VulkanRenderSystem.h"
#include "Rendering/Vulkan/Helpers/VKInitializers.h"

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace Puffin::Rendering
{
	void VKForwardRenderer::Setup(std::shared_ptr<VulkanRenderSystem> vulkanRenderSystem)
	{
		m_vulkanRenderSystem = vulkanRenderSystem;

		if (m_vulkanRenderSystem)
		{
			// Setup Frame Data
			m_forwardFrameData.clear();
			m_forwardFrameData.reserve(FRAME_OVERLAP);

			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				m_forwardFrameData.emplace_back();
			}

			// Initialize Extent
			m_renderExtent = { m_vulkanRenderSystem->m_offscreenExtent.width, m_vulkanRenderSystem->m_offscreenExtent.height, 1 };

			// Perform Setup
			SetupCommandBuffers();
			SetupSemaphores();
			SetupPipeline();
		}
	}

	VkSemaphore& VKForwardRenderer::DrawScene(const int& frameIndex)
	{
		if (m_vulkanRenderSystem)
		{
			VkCommandBuffer cmd = RecordCommandBuffer(frameIndex);

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;
			submit.pWaitDstStageMask = &waitStage;
			submit.waitSemaphoreCount = 0;
			submit.pWaitSemaphores = nullptr;
			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &m_forwardFrameData[frameIndex].renderSemaphore;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmd;

			VK_CHECK(vkQueueSubmit(m_vulkanRenderSystem->graphicsQueue, 1, &submit, VK_NULL_HANDLE));
		}

		return m_forwardFrameData[frameIndex].renderSemaphore;
	}

	void VKForwardRenderer::Cleanup()
	{
		m_mainDeletionQueue.flush();
	}

	void VKForwardRenderer::SetupCommandBuffers()
	{
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			// Create Alloc Info
			VkCommandBufferAllocateInfo allocInfo = VKInit::CommandBufferAllocateInfo(m_vulkanRenderSystem->m_frames[i].commandPool, 1);

			if (m_forwardFrameData[i].commandBuffer == VK_NULL_HANDLE)
			{
				VK_CHECK(vkAllocateCommandBuffers(m_vulkanRenderSystem->device, &allocInfo, &m_forwardFrameData[i].commandBuffer));
			}
		}
	}

	void VKForwardRenderer::SetupSemaphores()
	{
		// Create Geometry and Shading Semaphores
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		VkFenceCreateInfo fenceCreateInfo = VKInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		// For each overlapping frames data
		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			VK_CHECK(vkCreateSemaphore(m_vulkanRenderSystem->device, &semaphoreCreateInfo, nullptr, &m_forwardFrameData[i].renderSemaphore));

			m_mainDeletionQueue.push_function([=]()
			{
				vkDestroySemaphore(m_vulkanRenderSystem->device, m_forwardFrameData[i].renderSemaphore, nullptr);
			});
		}
	}

	void VKForwardRenderer::SetupPipeline()
	{
		const fs::path vertShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\forward_rendering_vert.spv";
		const fs::path fragShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\forward_rendering_frag.spv";

		// Read Shader code from Files
		auto vertShaderCode = ReadFile(vertShaderPath.string());
		auto fragShaderCode = ReadFile(fragShaderPath.string());

		// Create Shader Modules
		VkShaderModule vertShaderModule = VKInit::CreateShaderModule(m_vulkanRenderSystem->device, vertShaderCode);
		VkShaderModule fragShaderModule = VKInit::CreateShaderModule(m_vulkanRenderSystem->device, fragShaderCode);

		// Create Pipeline Layout Info
		std::vector<VkDescriptorSetLayout> setLayouts =
		{
			m_vulkanRenderSystem->m_cameraViewProjSetLayout,
			m_vulkanRenderSystem->m_objectInstanceSetLayout,
			m_vulkanRenderSystem->m_matTextureSetLayout
			
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

		VK_CHECK(vkCreatePipelineLayout(m_vulkanRenderSystem->device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

		// Pipeline Builder Object
		PipelineBuilder pipelineBuilder;

		// Create Shader Stage info
		pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
		pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
		pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// Create Vertex Input Info
		pipelineBuilder.vertexInputInfo = pipelineVertexInputStateCreateInfo;

		// Create Input Assembly Info
		pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		// Rasterization Stage Creation - Configured to draw filled triangles
		pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// Multisampled - Disabled right now so just use default
		pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

		// Color Blending - Default RGBA Color Blending
		VkPipelineColorBlendAttachmentState blendAttachState = VKInit::ColorBlendAttachmentState(0xf, VK_FALSE);

		pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(1, &blendAttachState);

		// Depth Testing - Default
		pipelineBuilder.depthStencil = VKInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		// Dynamic Viewport/Scissor Size
		pipelineBuilder.dynamic = VKInit::DynamicStateCreateInfo(dynamicStates);

		// Assign Pipeline Layout to Pipeline
		pipelineBuilder.pipelineLayout = m_pipelineLayout;

		// Build Pipeline
		m_pipeline = pipelineBuilder.BuildPipeline(m_vulkanRenderSystem->device, m_vulkanRenderSystem->m_renderPass);
	}

	VkCommandBuffer VKForwardRenderer::RecordCommandBuffer(const int& frameIndex)
	{
		VkCommandBuffer cmd = m_forwardFrameData[frameIndex].commandBuffer;

		// Setup Command Buffer Info
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;
		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// Set Clear Color
		std::array<VkClearValue, 2> clearValues;
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// Setup Render Pass Info
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = m_vulkanRenderSystem->m_renderPass;
		renderPassBeginInfo.framebuffer = m_vulkanRenderSystem->offscreenFramebuffers[frameIndex];
		renderPassBeginInfo.renderArea.extent.width = m_renderExtent.width;
		renderPassBeginInfo.renderArea.extent.height = m_renderExtent.height;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		// Begin Command Buffer Recording
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// Begin Render Pass
		vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Setup Viewport
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_renderExtent.width;
		viewport.height = (float)m_renderExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		// Setup Scissor
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = m_renderExtent.width;
		scissor.extent.height = m_renderExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Bind Pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		// Bind Descriptor Sets (Vertex)

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 0, 1, &m_vulkanRenderSystem->GetCurrentFrame().cameraViewProjDescriptor, 0, nullptr);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 1, 1, &m_vulkanRenderSystem->GetCurrentFrame().objectInstanceDescriptor, 0, nullptr);


		// Bind Descriptor Sets (Fragment)

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 2, 1, &m_vulkanRenderSystem->GetCurrentFrame().matTextureDescriptor, 0, nullptr);

		/*vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 3, 1, &m_vulkanRenderSystem->GetCurrentFrame().cameraShadingDescriptor, 0, nullptr);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 4, 1, &m_vulkanRenderSystem->GetCurrentFrame().lightDataDescriptor, 0, nullptr);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout, 5, 1, &m_vulkanRenderSystem->GetCurrentFrame().shadowmapDescriptor, 0, nullptr);*/


			// Bind Vertex/Index Buffers
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &m_vulkanRenderSystem->m_sceneRenderData.mergedVertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(cmd, m_vulkanRenderSystem->m_sceneRenderData.mergedIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Render Scene with Draw Indirect
		vkCmdDrawIndexedIndirect(cmd, m_vulkanRenderSystem->GetCurrentFrame().drawBatch.drawIndirectCommandsBuffer.buffer,
			0, m_vulkanRenderSystem->GetCurrentFrame().drawBatch.count, sizeof(VkDrawIndexedIndirectCommand));

		// End Render Pass
		vkCmdEndRenderPass(cmd);

		// End Command Buffer Recording
		VK_CHECK(vkEndCommandBuffer(cmd));

		return cmd;
	}
}
