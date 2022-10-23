#include "VKDeferredRender.h"

#include <Rendering/VKInitializers.h>
#include <Rendering/VKDebug.h>
#include <Rendering/VKHelper.h>

#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>

#include "Types/Vertex.hpp"

#include "Assets/AssetRegistry.h"

// STL
#include <iostream>
#include <cassert>
#include <array>
#include <array>
#include <string>

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

namespace Puffin
{
	namespace Rendering
	{
		// Public

		void VKDeferredRender::Setup(VkPhysicalDevice inPhysicalDevice,
		                             VkDevice inDevice,
		                             VmaAllocator inAllocator,
		                             VKUtil::DescriptorAllocator* inDescriptorAllocator,
		                             VKUtil::DescriptorLayoutCache* inDescriptorLayoutCache,
		                             std::vector<VkCommandPool>& commandPools,
		                             int inFrameOverlap,
		                             VkExtent2D inExtent)
		{
			// Pass in vulkan engine handles
			device = inDevice;
			physicalDevice = inPhysicalDevice;
			allocator = inAllocator;
			descriptorAllocator = inDescriptorAllocator;
			descriptorLayoutCache = inDescriptorLayoutCache;
			

			// Setup Frame Data
			frameOverlap = inFrameOverlap;

			frameData.clear();
			frameData.reserve(frameOverlap);

			for (int index = 0; index < frameOverlap; index++)
			{
				frameData.emplace_back();
			}

			gBufferExtent = { inExtent.width, inExtent.height, 1 };

			SetupCommandBuffers(commandPools);
		}

		void VKDeferredRender::SetupGeometry(VkDescriptorSetLayout& inCameraVPSetLayout, VkDescriptorSetLayout& inObjectInstanceSetLayout, VkDescriptorSetLayout& inMatTextureSetLayout)
		{
			m_cameraVPLayout = inCameraVPSetLayout;
			m_objectInstanceLayout = inObjectInstanceSetLayout;
			m_matTextureLayout = inMatTextureSetLayout;

			// Setup GBuffer framebuffer, attachments, render pass and color sampler
			SetupGBuffer();
			SetupGRenderPass();
			SetupGFramebuffer();
			SetupSynchronization();
			SetupGColorSampler();
			SetupGPipeline();
		}

		void VKDeferredRender::SetupShading(VkRenderPass inRenderPass, VkDescriptorSetLayout& inCameraShadingSetLayout, VkDescriptorSetLayout& inLightSetLayout, VkDescriptorSetLayout& inShadowmapSetLayout)
		{
			sRenderPass = inRenderPass;
			m_cameraShadingLayout = inCameraShadingSetLayout;
			m_lightLayout = inLightSetLayout;
			m_shadowmapLayout = inShadowmapSetLayout;

			// Setup Shading Structures
			SetupGBufferDescriptorSets();
			SetupSPipeline();
		}

		void VKDeferredRender::RecreateFramebuffer(VkExtent2D inExtent)
		{
			m_framebufferDeletionQueue.flush();

			gBufferExtent = { inExtent.width, inExtent.height, 1 };

			SetupGBuffer();
			SetupGFramebuffer();
			UpdateGBufferDescriptorSets();
		}

		VkSemaphore& VKDeferredRender::DrawScene(int frameIndex, SceneRenderData* sceneData, VkQueue graphicsQueue, VkFramebuffer sFramebuffer, VkSemaphore& shadowmapWaitSemaphore)
		{
			// 1. Geometry Pass: Render all geometric/color data to g-buffer
			VkCommandBuffer cmdGeometry = RecordGeometryCommandBuffer(frameIndex, sceneData);

			// Submit Geometry Command Buffer
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo submit = {};
			submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit.pNext = nullptr;
			submit.pWaitDstStageMask = &waitStage;
			submit.waitSemaphoreCount = 0;
			submit.pWaitSemaphores = nullptr;
			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &frameData[frameIndex].geometrySemaphore;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmdGeometry;

			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE));

			// 2. Lighting Pass: Use G-Buffer to calculate the scenes lighting
			VkCommandBuffer cmdShading = RecordShadingCommandBuffer(frameIndex, sceneData, sFramebuffer);

			std::vector<VkSemaphore> waitSemaphores =
			{
				frameData[frameIndex].geometrySemaphore/*,
				shadowmapWaitSemaphore*/
			};

			// Submit Shading Command Buffer
			submit.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
			submit.pWaitSemaphores = waitSemaphores.data();
			submit.signalSemaphoreCount = 1;
			submit.pSignalSemaphores = &frameData[frameIndex].shadingSemaphore;
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmdShading;
			VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, VK_NULL_HANDLE));

			// Return Shading Semaphore so Vulkan Engine can know when shading is done
			return frameData[frameIndex].shadingSemaphore;
		}

		void VKDeferredRender::Cleanup()
		{
			m_deletionQueue.flush();
			m_framebufferDeletionQueue.flush();
		}

		//-------------------------------------------------------------------------------------
		// Private
		//-------------------------------------------------------------------------------------

		//-------------------------------------------------------------------------------------
		// Setup
		//-------------------------------------------------------------------------------------

		void VKDeferredRender::SetupGBuffer()
		{
			// For each overlapping frames data
			for (uint32_t i = 0; i < frameOverlap; i++)
			{
				// Color Attachments

				// (World Space) Positions
				CreateAllocatedImage(
					VK_FORMAT_R32G32B32A32_SFLOAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					&frameData[i].gPosition,
					"GBuffer - Positions " + std::to_string(i));

				// (World Space) Normals
				CreateAllocatedImage(
					VK_FORMAT_R16G16B16A16_SFLOAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					&frameData[i].gNormal,
					"GBuffer - Normals " + std::to_string(i));

				// Albedo/Specular (Color/Light Reflection)
				CreateAllocatedImage(
					VK_FORMAT_R8G8B8A8_UNORM,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					&frameData[i].gAlbedoSpec,
					"GBuffer - Albedo/Specular " + std::to_string(i));

				// Depth Attachment

				// Find Suitable Depth Format
				VkFormat depthFormat;
				VkBool32 bFoundValidDepthFormat = VKHelper::GetSupportedDepthFormat(physicalDevice, &depthFormat);

				// If bFoundValidDepth format is false, a suitable depth format was not found
				assert(bFoundValidDepthFormat);

				// Create Depth Attachment
				CreateAllocatedImage(depthFormat,
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					&frameData[i].gDepth,
					"GBuffer - Depth " + std::to_string(i));
			}
		}

		void VKDeferredRender::SetupGRenderPass()
		{
			const int numAttachments = 4;

			// Setup Descriptions for Position, Normal, AlbedoSpec and Depth Attachments
			std::array<VkAttachmentDescription, numAttachments> attachmentDescs = {};

			// Attachment Properties
			for (uint32_t i = 0; i < numAttachments; i++)
			{
				attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				// Layout is different for depth attachment
				if (i == 3)
				{
					attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}

			// Formats
			attachmentDescs[0].format = frameData[0].gPosition.format;
			attachmentDescs[1].format = frameData[0].gNormal.format;
			attachmentDescs[2].format = frameData[0].gAlbedoSpec.format;
			attachmentDescs[3].format = frameData[0].gDepth.format;

			std::vector<VkAttachmentReference> colorRefs;
			colorRefs.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorRefs.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorRefs.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkAttachmentReference depthRef = {};
			depthRef.attachment = 3;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Subpasses
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = colorRefs.data();
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
			subpass.pDepthStencilAttachment = &depthRef;

			// Use subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			// Render Pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescs.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();

			VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &gRenderPass));

			m_deletionQueue.push_function([=]()
			{
				vkDestroyRenderPass(device, gRenderPass, nullptr);
			});

		}

		void VKDeferredRender::SetupGFramebuffer()
		{
			// For each overlapping frames data
			for (uint32_t i = 0; i < frameOverlap; i++)
			{
				// Pack GBuffer Attachments into Array
				std::array<VkImageView, 4> attachments;
				attachments[0] = frameData[i].gPosition.imageView;
				attachments[1] = frameData[i].gNormal.imageView;
				attachments[2] = frameData[i].gAlbedoSpec.imageView;
				attachments[3] = frameData[i].gDepth.imageView;

				// Create Framebuffer with Attachments
				VkFramebufferCreateInfo frameBufferCreateInfo = {};
				frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				frameBufferCreateInfo.pNext = NULL;
				frameBufferCreateInfo.renderPass = gRenderPass;
				frameBufferCreateInfo.pAttachments = attachments.data();
				frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				frameBufferCreateInfo.width = gBufferExtent.width;
				frameBufferCreateInfo.height = gBufferExtent.height;
				frameBufferCreateInfo.layers = 1;

				VK_CHECK(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameData[i].gFramebuffer));

				m_framebufferDeletionQueue.push_function([=]()
				{
					vkDestroyFramebuffer(device, frameData[i].gFramebuffer, nullptr);
				});
			}
		}

		void VKDeferredRender::SetupSynchronization()
		{
			// Create Geometry and Shading Semaphores
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = nullptr;
			semaphoreCreateInfo.flags = 0;

			VkFenceCreateInfo fenceCreateInfo = VKInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

			// For each overlapping frames data
			for (uint32_t i = 0; i < frameOverlap; i++)
			{
				VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo,
					nullptr, &frameData[i].geometrySemaphore));

				VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo,
					nullptr, &frameData[i].shadingSemaphore));

				m_deletionQueue.push_function([=]()
				{
					vkDestroySemaphore(device, frameData[i].geometrySemaphore, nullptr);
					vkDestroySemaphore(device, frameData[i].shadingSemaphore, nullptr);
				});
			}
		}

		void VKDeferredRender::SetupGColorSampler()
		{
			// Create sampler to sample from color attachments
			VkSamplerCreateInfo samplerCreateInfo = VKInit::SamplerCreateInfo(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = 1.0f;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &gColorSampler));
		}

		void VKDeferredRender::SetupCommandBuffers(std::vector<VkCommandPool>& commandPools)
		{
			// For each overlapping frames data
			for (uint32_t i = 0; i < frameOverlap; i++)
			{
				// Create Alloc Info (Same for both buffers)
				VkCommandBufferAllocateInfo allocInfo = VKInit::CommandBufferAllocateInfo(commandPools[i], 1);

				// Only Allocate Command Buffers if they are NULL
				if (frameData[i].gCommandBuffer == VK_NULL_HANDLE)
				{
					VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frameData[i].gCommandBuffer));
				}

				if (frameData[i].sCommandBuffer == VK_NULL_HANDLE)
				{
					VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frameData[i].sCommandBuffer));
				}
			}
		}

		void VKDeferredRender::SetupGPipeline()
		{
			const fs::path vertShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\deferred_geometry_vert.spv";
			const fs::path fragShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\deferred_geometry_frag.spv";

			// Read Shader code from Files
			auto vertShaderCode = ReadFile(vertShaderPath.string());
			auto fragShaderCode = ReadFile(fragShaderPath.string());

			// Create Shader Modules
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_cameraVPLayout,
				m_objectInstanceLayout,
				m_matTextureLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gPipelineLayout));

			// Pipeline Builder Object
			PipelineBuilder pipelineBuilder;

			// Create Shader Stage info
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
			pipelineBuilder.shaderStages.push_back(VKInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

			VkVertexInputBindingDescription bindingDescription;
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			Vertex_PNTV_32::GetVertexBindingAndAttributes(bindingDescription, attributeDescriptions);

			// Create Vertex Input Info
			pipelineBuilder.vertexInputInfo = VKInit::VertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

			// Create Input Assembly Info
			pipelineBuilder.inputAssembly = VKInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			// Rasterization Stage Creation - Configured to draw filled triangles
			pipelineBuilder.rasterizer = VKInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);

			// Multisampled - Disabled right now so just use default
			pipelineBuilder.multisampling = VKInit::MultisamplingStateCreateInfo();

			// Color Blending - Default RGBA Color Blending
			std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates =
			{
				VKInit::ColorBlendAttachmentState(0xf, VK_FALSE),
				VKInit::ColorBlendAttachmentState(0xf, VK_FALSE),
				VKInit::ColorBlendAttachmentState(0xf, VK_FALSE)
			};

			pipelineBuilder.colorBlendCreateInfo = VKInit::ColorBlendStateCreateInfo(
				static_cast<uint32_t>(blendAttachmentStates.size()),
				blendAttachmentStates.data());

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
			pipelineBuilder.pipelineLayout = gPipelineLayout;

			// Build Pipeline
			gPipeline = pipelineBuilder.BuildPipeline(device, gRenderPass);
		}

		void VKDeferredRender::SetupGBufferDescriptorSets()
		{
			for (int i = 0; i < frameOverlap; i++)
			{
				// G-Buffer Info
				VkDescriptorImageInfo gPositionInfo;
				gPositionInfo.sampler = gColorSampler;
				gPositionInfo.imageView = frameData[i].gPosition.imageView;
				gPositionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo gNormalInfo;
				gNormalInfo.sampler = gColorSampler;
				gNormalInfo.imageView = frameData[i].gNormal.imageView;
				gNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo gAlbedoSpecInfo;
				gAlbedoSpecInfo.sampler = gColorSampler;
				gAlbedoSpecInfo.imageView = frameData[i].gAlbedoSpec.imageView;
				gAlbedoSpecInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.BindImage(1, &gPositionInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindImage(2, &gNormalInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.BindImage(3, &gAlbedoSpecInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build(frameData[i].gBufferDescriptor, m_gBufferLayout);
			}
		}

		void VKDeferredRender::SetupSPipeline()
		{
			const fs::path vertShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\deferred_shading_vert.spv";
			const fs::path fragShaderPath = Assets::AssetRegistry::Get()->ContentRoot() / "shaders\\deferred_shading_frag.spv";

			// Read Shader code from Files
			auto vertShaderCode = ReadFile(vertShaderPath.string());
			auto fragShaderCode = ReadFile(fragShaderPath.string());

			// Create Shader Modules
			VkShaderModule vertShaderModule = VKInit::CreateShaderModule(device, vertShaderCode);
			VkShaderModule fragShaderModule = VKInit::CreateShaderModule(device, fragShaderCode);

			// Create Pipeline Layout Info
			std::vector<VkDescriptorSetLayout> setLayouts =
			{
				m_cameraShadingLayout,
				m_gBufferLayout,
				m_lightLayout,
				m_shadowmapLayout
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = VKInit::PipelineLayoutCreateInfo(setLayouts);

			VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &sPipelineLayout));

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
			pipelineBuilder.pipelineLayout = sPipelineLayout;

			// Build Pipeline
			sPipeline = pipelineBuilder.BuildPipeline(device, sRenderPass);
		}

		void VKDeferredRender::UpdateGBufferDescriptorSets()
		{
			for (int i = 0; i < frameOverlap; i++)
			{
				// G-Buffer Info
				VkDescriptorImageInfo gPositionInfo;
				gPositionInfo.sampler = gColorSampler;
				gPositionInfo.imageView = frameData[i].gPosition.imageView;
				gPositionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo gNormalInfo;
				gNormalInfo.sampler = gColorSampler;
				gNormalInfo.imageView = frameData[i].gNormal.imageView;
				gNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkDescriptorImageInfo gAlbedoSpecInfo;
				gAlbedoSpecInfo.sampler = gColorSampler;
				gAlbedoSpecInfo.imageView = frameData[i].gAlbedoSpec.imageView;
				gAlbedoSpecInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VKUtil::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
					.UpdateImage(1, &gPositionInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.UpdateImage(2, &gNormalInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.UpdateImage(3, &gAlbedoSpecInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					.Update(frameData[i].gBufferDescriptor);
			}
		}

		void VKDeferredRender::CreateAllocatedImage(VkFormat format, VkImageUsageFlags usage, AllocatedImage* allocatedImage, std::string debug_name)
		{
			// Set Image format
			allocatedImage->format = format;

			// Get Image Create Info
			VkImageCreateInfo imageInfo = VKInit::ImageCreateInfo(allocatedImage->format, usage, gBufferExtent);

			// Get Image Allocation Info
			VmaAllocationCreateInfo imageAllocInfo = {};
			imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			imageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Create Image
			vmaCreateImage(allocator, &imageInfo, &imageAllocInfo,
				&allocatedImage->image, &allocatedImage->allocation, nullptr);

			// Set optional name for RenderDoc debugging
			if (debug_name != "")
			{
				VKDebug::SetObjectName(device,
					(uint64_t)allocatedImage->image,
					VK_OBJECT_TYPE_IMAGE,
					debug_name.c_str());
			}

			// Get aspect flag for either color or depth image
			VkImageAspectFlags aspectFlags = 0;

			if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			// Create Image View
			VkImageViewCreateInfo imageViewInfo = VKInit::ImageViewCreateInfo(allocatedImage->format,
				allocatedImage->image, aspectFlags);

			VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &allocatedImage->imageView));

			// Add image/view to deletion queue for cleanup
			m_framebufferDeletionQueue.push_function([=]()
			{
				vkDestroyImageView(device, allocatedImage->imageView, nullptr);
				vmaDestroyImage(allocator, allocatedImage->image, allocatedImage->allocation);
			});
		}

		//-------------------------------------------------------------------------------------
		// Draw
		//-------------------------------------------------------------------------------------

		VkCommandBuffer VKDeferredRender::RecordGeometryCommandBuffer(int frameIndex, SceneRenderData* sceneData)
		{
			VkCommandBuffer cmd = frameData[frameIndex].gCommandBuffer;

			// Setup Command Buffer Info
			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;
			cmdBeginInfo.pInheritanceInfo = nullptr;
			cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			// Set Clear Colors for Framebuffer
			std::array<VkClearValue, 4> clearValues;
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
			clearValues[1].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
			clearValues[2].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
			clearValues[3].depthStencil = { 1.0f, 0 };

			// Setup Render Pass Info
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = gRenderPass;
			renderPassBeginInfo.framebuffer = frameData[frameIndex].gFramebuffer;
			renderPassBeginInfo.renderArea.extent.width = gBufferExtent.width;
			renderPassBeginInfo.renderArea.extent.height = gBufferExtent.height;
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
			viewport.width = (float)gBufferExtent.width;
			viewport.height = (float)gBufferExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			// Setup Scissor
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent.width = gBufferExtent.width;
			scissor.extent.height = gBufferExtent.height;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			// Bind Pipeline
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline);

			// Bind Geometry Descriptor Sets
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				gPipelineLayout, 0, 1, m_cameraVPSet, 0, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				gPipelineLayout, 1, 1, m_modelInstanceSet, 0, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				gPipelineLayout, 2, 1, m_matTextureSet, 0, nullptr);

			// Bind Vertex/Index Buffers
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &sceneData->mergedVertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmd, sceneData->mergedIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			// Render Scene with Draw Indirect
			vkCmdDrawIndexedIndirect(cmd, indirectDrawBatch->drawIndirectCommandsBuffer.buffer,
				0, indirectDrawBatch->count, sizeof(VkDrawIndexedIndirectCommand));

			// End Render Pass
			vkCmdEndRenderPass(cmd);

			// End Command Buffer Recording
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}

		VkCommandBuffer VKDeferredRender::RecordShadingCommandBuffer(int frameIndex, SceneRenderData* sceneData, VkFramebuffer sFramebuffer)
		{
			VkCommandBuffer cmd = frameData[frameIndex].sCommandBuffer;

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
			renderPassBeginInfo.renderPass = sRenderPass;
			renderPassBeginInfo.framebuffer = sFramebuffer;
			renderPassBeginInfo.renderArea.extent.width = gBufferExtent.width;
			renderPassBeginInfo.renderArea.extent.height = gBufferExtent.height;
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
			viewport.width = (float)gBufferExtent.width;
			viewport.height = (float)gBufferExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vkCmdSetViewport(cmd, 0, 1, &viewport);

			// Setup Scissor
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent.width = gBufferExtent.width;
			scissor.extent.height = gBufferExtent.height;

			vkCmdSetScissor(cmd, 0, 1, &scissor);

			// Bind Pipeline
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sPipeline);

			// Bind Shading Geometry Set
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				sPipelineLayout, 0, 1, m_cameraShadingSet, 0, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				sPipelineLayout, 1, 1, &frameData[frameIndex].gBufferDescriptor, 0, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				sPipelineLayout, 2, 1, m_lightDataSet, 0, nullptr);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				sPipelineLayout, 3, 1, m_shadowmapSet, 0, nullptr);

			vkCmdDraw(cmd, 3, 1, 0, 0);

			// End Render Pass
			vkCmdEndRenderPass(cmd);

			// End Command Buffer Recording
			VK_CHECK(vkEndCommandBuffer(cmd));

			return cmd;
		}
	}
}