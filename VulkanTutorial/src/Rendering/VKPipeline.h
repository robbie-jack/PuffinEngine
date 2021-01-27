#pragma once

#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include <vulkan/vulkan.h>

// STL
#include <vector>
#include <iostream>

namespace Puffin
{
	namespace Rendering
	{
		class PipelineBuilder
		{
		public:

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			VkPipelineVertexInputStateCreateInfo vertexInputInfo;
			VkPipelineInputAssemblyStateCreateInfo inputAssembly;
			VkViewport viewport;
			VkRect2D scissor;
			VkPipelineRasterizationStateCreateInfo rasterizer;
			VkPipelineColorBlendAttachmentState colorBlendAttachment;
			VkPipelineMultisampleStateCreateInfo multisampling;
			VkPipelineDepthStencilStateCreateInfo depthStencil;
			VkPipelineDynamicStateCreateInfo dynamic;
			VkPipelineLayout pipelineLayout;

			VkPipeline build_pipeline(VkDevice device, VkRenderPass pass)
			{
				// Make viewport state from our stored viewport and scissor
				VkPipelineViewportStateCreateInfo viewportState = {};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.pNext = nullptr;

				viewportState.viewportCount = 1;
				viewportState.pViewports = &viewport;
				viewportState.scissorCount = 1;
				viewportState.pScissors = &scissor;

				// Setup dummy color blending. We aren't using transparent objects yet
				// the blendind is just no blend, but we do write to the color attachment
				VkPipelineColorBlendStateCreateInfo colorBlending = {};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.pNext = nullptr;

				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = 1;
				colorBlending.pAttachments = &colorBlendAttachment;

				// Build the actual pipeline
				// we now use all of the info structs we have been writing into this one to create the pipeline
				VkGraphicsPipelineCreateInfo pipelineInfo = {};
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.pNext = nullptr;

				pipelineInfo.stageCount = shaderStages.size();
				pipelineInfo.pStages = shaderStages.data();
				pipelineInfo.pVertexInputState = &vertexInputInfo;
				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pMultisampleState = &multisampling;
				pipelineInfo.pColorBlendState = &colorBlending;
				pipelineInfo.pDepthStencilState = &depthStencil;
				pipelineInfo.pDynamicState = &dynamic;
				pipelineInfo.layout = pipelineLayout;
				pipelineInfo.renderPass = pass;
				pipelineInfo.subpass = 0;
				pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

				//its easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
				VkPipeline newPipeline;
				if (vkCreateGraphicsPipelines(
					device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
					std::cout << "failed to create pipline\n";
					return VK_NULL_HANDLE; // failed to create graphics pipeline
				}
				else
				{
					return newPipeline;
				}
			}
		};
	}
}

#endif // VULKAN_PIPELINE_H