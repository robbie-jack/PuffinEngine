#pragma once

#ifndef VULKAN_INITIALIZERS_H
#define VULKAN_INITIALIZERS_H

#include "VKTypes.h"

namespace Puffin
{
	namespace Rendering
	{
		namespace vkinit
		{
			VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
			{
				VkCommandPoolCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				info.pNext = nullptr;

				info.queueFamilyIndex = queueFamilyIndex;
				info.flags = flags;
				return info;
			}

			VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
			{
				VkCommandBufferAllocateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.pNext = nullptr;

				info.commandPool = pool;
				info.commandBufferCount = count;
				info.level = level;
				return info;
			}

			VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags)
			{
				VkCommandBufferBeginInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				info.pNext = nullptr;

				info.pInheritanceInfo = nullptr;
				info.flags = flags;
				return info;
			}

			VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0)
			{
				VkFenceCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				info.pNext = nullptr;

				info.flags = flags;

				return info;
			}

			VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0)
			{
				VkSemaphoreCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				info.pNext = nullptr;
				info.flags = flags;
				return info;
			}

			VkSubmitInfo submit_info(VkCommandBuffer* cmd)
			{
				VkSubmitInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				info.pNext = nullptr;

				info.waitSemaphoreCount = 0;
				info.pWaitSemaphores = nullptr;
				info.pWaitDstStageMask = nullptr;
				info.commandBufferCount = 1;
				info.pCommandBuffers = cmd;
				info.signalSemaphoreCount = 0;
				info.pSignalSemaphores = nullptr;

				return info;
			}

			VkPresentInfoKHR present_info()
			{
				VkPresentInfoKHR info = {};
				info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				info.pNext = nullptr;

				info.swapchainCount = 0;
				info.pSwapchains = nullptr;
				info.pWaitSemaphores = nullptr;
				info.waitSemaphoreCount = 0;
				info.pImageIndices = nullptr;

				return info;
			}

			// Initialize Shader Module
			VkShaderModule create_shader_module(VkDevice device, const std::vector<char>& code)
			{
				VkShaderModuleCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = code.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

				VkShaderModule shaderModule;
				if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to create shader module!");
				}

				return shaderModule;
			}

			// Initialize infor for one shader stage
			VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) 
			{
				VkPipelineShaderStageCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				info.pNext = nullptr;

				//shader stage
				info.stage = stage;
				//module containing the code for this shader stage
				info.module = shaderModule;
				//the entry point of the shader
				info.pName = "main";
				return info;
			}

			// Initialize info for vertex buffers and formats
			VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(
				VkVertexInputBindingDescription bindingDescription, 
				std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions)
			{
				VkPipelineVertexInputStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				info.pNext = nullptr;

				//no vertex bindings or attributes
				info.vertexBindingDescriptionCount = 1;
				info.vertexAttributeDescriptionCount = attributeDescriptions.size();
				info.pVertexBindingDescriptions = &bindingDescription;
				info.pVertexAttributeDescriptions = attributeDescriptions.data();
				return info;
			}

			// Initialize info for what king of topology to draw, i.e triangles, lines, points
			VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology) 
			{
				VkPipelineInputAssemblyStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				info.pNext = nullptr;

				info.topology = topology;
				//we are not going to use primitive restart on the entire tutorial so leave it on false
				info.primitiveRestartEnable = VK_FALSE;
				return info;
			}

			// Initialize info for rasterization i.i backface culling, line width, wireframe drawing
			VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode)
			{
				VkPipelineRasterizationStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				info.pNext = nullptr;

				info.depthClampEnable = VK_FALSE;
				//rasterizer discard allows objects with holes, default to no
				info.rasterizerDiscardEnable = VK_FALSE;

				info.polygonMode = polygonMode;
				info.lineWidth = 1.0f;
				//no backface cull
				info.cullMode = VK_CULL_MODE_NONE;
				info.frontFace = VK_FRONT_FACE_CLOCKWISE;
				//no depth bias
				info.depthBiasEnable = VK_FALSE;
				info.depthBiasConstantFactor = 0.0f;
				info.depthBiasClamp = 0.0f;
				info.depthBiasSlopeFactor = 0.0f;

				return info;
			}

			// Initialize info for multisampling
			VkPipelineMultisampleStateCreateInfo multisampling_state_create_info()
			{
				VkPipelineMultisampleStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				info.pNext = nullptr;

				info.sampleShadingEnable = VK_FALSE;
				//multisampling defaulted to no multisampling (1 sample per pixel)
				info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				info.minSampleShading = 1.0f;
				info.pSampleMask = nullptr;
				info.alphaToCoverageEnable = VK_FALSE;
				info.alphaToOneEnable = VK_FALSE;
				return info;
			}

			// Initialie info for color vlending
			VkPipelineColorBlendAttachmentState color_blend_attachment_state() 
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = VK_FALSE;
				return colorBlendAttachment;
			}

			// Initialize Pipeline Layout
			VkPipelineLayoutCreateInfo pipeline_layout_create_info(VkDescriptorSetLayout& layout) 
			{
				VkPipelineLayoutCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				info.pNext = nullptr;

				//empty defaults
				info.flags = 0;
				info.setLayoutCount = 1;
				info.pSetLayouts = &layout;
				info.pushConstantRangeCount = 0;
				info.pPushConstantRanges = nullptr;
				return info;
			}

			VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
			{
				VkImageCreateInfo info = { };
				info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				info.pNext = nullptr;

				info.imageType = VK_IMAGE_TYPE_2D;

				info.format = format;
				info.extent = extent;

				info.mipLevels = 1;
				info.arrayLayers = 1;
				info.samples = VK_SAMPLE_COUNT_1_BIT;
				info.tiling = VK_IMAGE_TILING_OPTIMAL;
				info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				info.usage = usageFlags;

				return info;
			}

			VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
			{
				//build a image-view for the image to use for rendering
				VkImageViewCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				info.pNext = nullptr;

				info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				info.image = image;
				info.format = format;
				info.subresourceRange.baseMipLevel = 0;
				info.subresourceRange.levelCount = 1;
				info.subresourceRange.baseArrayLayer = 0;
				info.subresourceRange.layerCount = 1;
				info.subresourceRange.aspectMask = aspectFlags;

				return info;
			}

			VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp)
			{
				VkPipelineDepthStencilStateCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				info.pNext = nullptr;

				info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
				info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
				info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
				info.depthBoundsTestEnable = VK_FALSE;
				info.minDepthBounds = 0.0f; // Optional
				info.maxDepthBounds = 1.0f; // Optional
				info.stencilTestEnable = VK_FALSE;

				return info;
			}

			VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
			{
				VkSamplerCreateInfo info = {};
				info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				info.pNext = nullptr;

				info.magFilter = filters;
				info.minFilter = filters;
				info.addressModeU = samplerAddressMode;
				info.addressModeV = samplerAddressMode;
				info.addressModeW = samplerAddressMode;

				return info;
			}
		}
	}
}

#endif // VULKAN_INITIALIZERS_H