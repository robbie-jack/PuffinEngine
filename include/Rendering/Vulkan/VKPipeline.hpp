#pragma once

#include <fstream>
#include <vulkan/vulkan.hpp>

#include "VKVertex.hpp"

namespace Puffin::Rendering::VK::Util
{
	class ShaderModule
	{
	public:

		ShaderModule() {}

		ShaderModule(const vk::Device& device, const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				return;
			}

			// find what the size of the file is by looking up the location of the cursor
			// because the cursor is at the end, it gives the size directly in bytes
			size_t fileSize = (size_t)file.tellg();

			// spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
			std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

			// put fiel cursor at beginning
			file.seekg(0);

			// load entire file into buffer
			file.read((char*)buffer.data(), fileSize);

			file.close();

			vk::ShaderModuleCreateInfo createInfo = { {}, buffer.size() * sizeof(uint32_t), buffer.data() };

			m_module = device.createShaderModuleUnique(&createInfo, nullptr);

			m_successful = true;
		}

		bool Successful() const { return m_successful; }
		vk::UniqueShaderModule Module() const { return m_module; }

	private:

		vk::UniqueShaderModule m_module;
		bool m_successful = false;

	};

	class PipelineLayoutBuilder
	{
	public:

		static PipelineLayoutBuilder Begin() { return PipelineLayoutBuilder(); }

		PipelineLayoutBuilder& DescriptorSetLayout(vk::DescriptorSetLayout layout)
		{
			m_setLayouts.push_back(layout);
			return *this;
		}

		vk::UniquePipelineLayout CreateUnique(const vk::Device& device)
		{
			vk::PipelineLayoutCreateInfo layoutInfo = { {},
				static_cast<uint32_t>(m_setLayouts.size()), m_setLayouts.data() };

			return device.createPipelineLayoutUnique(layoutInfo);
		}

	private:

		std::vector<vk::DescriptorSetLayout> m_setLayouts;

	};

	class PipelineBuilder
	{
	public:

		PipelineBuilder(uint32_t width, uint32_t height)
		{
			Viewport(vk::Viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f });
			Scissor(vk::Rect2D{ {0, 0}, {width, height} });

			Init();
		}

		static PipelineBuilder Begin(uint32_t width, uint32_t height)
		{
			return PipelineBuilder(width, height);
		}

		void Init()
		{
			m_inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
			m_rasterizationState.lineWidth = 1.0f;

			// Set up depth test, but do not enable it.
			m_depthStencilState.depthTestEnable = VK_FALSE;
			m_depthStencilState.depthWriteEnable = VK_TRUE;
			m_depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
			m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
			m_depthStencilState.back.failOp = vk::StencilOp::eKeep;
			m_depthStencilState.back.passOp = vk::StencilOp::eKeep;
			m_depthStencilState.back.compareOp = vk::CompareOp::eAlways;
			m_depthStencilState.stencilTestEnable = VK_FALSE;
			m_depthStencilState.front = m_depthStencilState.back;
		}

		PipelineBuilder& Viewport(const vk::Viewport& viewport)
		{
			m_viewports.push_back(viewport);
			return *this;
		}

		PipelineBuilder& Scissor(const vk::Rect2D& scissor)
		{
			m_scissors.push_back(scissor);
			return *this;
		}

		PipelineBuilder& VertexLayout(const VertexLayout& vertexLayout)
		{
			m_vertexBindingDescriptions = vertexLayout.Bindings();
			m_vertexAttributeDescriptions = vertexLayout.Attributes();

			return *this;
		}

		PipelineBuilder& VertexBinding(uint32_t binding, uint32_t stride)
		{
			m_vertexBindingDescriptions.emplace_back(binding, stride);

			return *this;
		}

		PipelineBuilder& AttributeBinding(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset)
		{
			m_vertexAttributeDescriptions.emplace_back(location, binding, format, offset);

			return *this;
		}

		vk::UniquePipeline CreateUnique(const vk::Device& device,
			const vk::PipelineCache& pipelineCache,
			const vk::PipelineLayout& pipelineLayout,
			const vk::RenderPass& renderPass, bool defaultBlend = true)
		{
			
		}

	private:

		std::vector<vk::Viewport> m_viewports;
		std::vector<vk::Rect2D> m_scissors;

		vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyState;
		vk::PipelineRasterizationStateCreateInfo m_rasterizationState;
		vk::PipelineDepthStencilStateCreateInfo m_depthStencilState;
		vk::PipelineColorBlendStateCreateInfo m_colorBlendState;
		vk::PipelineTessellationStateCreateInfo m_tessellationState;
		std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
		std::vector<vk::PipelineShaderStageCreateInfo> m_modules;
		//std::vector<std::unique_ptr<SpecData>> m_moduleSpecializations;
		std::vector<vk::VertexInputAttributeDescription> m_vertexAttributeDescriptions;
		std::vector<vk::VertexInputBindingDescription> m_vertexBindingDescriptions;
		std::vector<vk::DynamicState> m_dynamicState;
		uint32_t m_subpass = 0;
	};
}