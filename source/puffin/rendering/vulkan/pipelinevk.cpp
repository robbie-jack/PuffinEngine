#include "puffin/rendering/vulkan/pipelinevk.h"

#include <fstream>

namespace puffin::rendering::util
{
	ShaderModule::ShaderModule(const vk::Device& device, const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			return;
		}

		// find what the size of the file is by looking up the location of the cursor
		// because the cursor is at the end, it gives the size directly in bytes
		const size_t fileSize = file.tellg();

		// spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		// put file cursor at beginning
		file.seekg(0);

		// load entire file into buffer
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		file.close();

		const vk::ShaderModuleCreateInfo createInfo = { {}, buffer.size() * sizeof(uint32_t), buffer.data() };

		mModule = device.createShaderModuleUnique(createInfo, nullptr);

		mSuccessful = true;
	}

	ShaderModule::ShaderModule(const vk::Device& device, const std::vector<uint32_t>& code)
	{
		const vk::ShaderModuleCreateInfo createInfo = { {}, code.size() * sizeof(uint32_t), code.data() };

		mModule = device.createShaderModuleUnique(createInfo, nullptr);

		mSuccessful = true;
	}

	bool ShaderModule::Successful() const
	{
		return mSuccessful;
	}

	vk::ShaderModule ShaderModule::Module() const
	{
		return mModule.get();
	}

	PipelineLayoutBuilder PipelineLayoutBuilder::Begin()
	{
		return PipelineLayoutBuilder();
	}

	PipelineLayoutBuilder& PipelineLayoutBuilder::DescriptorSetLayout(vk::DescriptorSetLayout layout)
	{
		mSetLayouts.push_back(layout);
		return *this;
	}

	PipelineLayoutBuilder& PipelineLayoutBuilder::PushConstantRange(vk::PushConstantRange range)
	{
		mPushRanges.push_back(range);
		return *this;
	}

	vk::UniquePipelineLayout PipelineLayoutBuilder::CreateUnique(const vk::Device& device)
	{
		const vk::PipelineLayoutCreateInfo layoutInfo = { {},
			static_cast<uint32_t>(mSetLayouts.size()), mSetLayouts.data(),	
			static_cast<uint32_t>(mPushRanges.size()), mPushRanges.data() };

		return device.createPipelineLayoutUnique(layoutInfo);
	}

	PipelineBuilder::PipelineBuilder(uint32_t width, uint32_t height)
	{
		Viewport(vk::Viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f });
		Scissor(vk::Rect2D{ {0, 0}, {width, height} });

		Init();
	}

	PipelineBuilder PipelineBuilder::Begin(uint32_t width, uint32_t height)
	{
		return PipelineBuilder(width, height);
	}

	void PipelineBuilder::Init()
	{
		mInputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		mRasterizationState.lineWidth = 1.0f;
		mRasterizationState.setCullMode(vk::CullModeFlagBits::eBack);

		// Set up depth test, but do not enable it.
		mDepthStencilState.depthTestEnable = VK_FALSE;
		mDepthStencilState.depthWriteEnable = VK_TRUE;
		mDepthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
		mDepthStencilState.depthBoundsTestEnable = VK_FALSE;
		mDepthStencilState.back.failOp = vk::StencilOp::eKeep;
		mDepthStencilState.back.passOp = vk::StencilOp::eKeep;
		mDepthStencilState.back.compareOp = vk::CompareOp::eAlways;
		mDepthStencilState.stencilTestEnable = VK_FALSE;
		mDepthStencilState.front = mDepthStencilState.back;
	}

	PipelineBuilder& PipelineBuilder::Viewport(const vk::Viewport& viewport)
	{
		mViewports.push_back(viewport);
		return *this;
	}

	PipelineBuilder& PipelineBuilder::Scissor(const vk::Rect2D& scissor)
	{
		mScissors.push_back(scissor);
		return *this;
	}

	PipelineBuilder& PipelineBuilder::VertexLayout(const util::VertexLayout& vertexLayout)
	{
		mVertexBindingDescriptions = vertexLayout.Bindings();
		mVertexAttributeDescriptions = vertexLayout.Attributes();

		return *this;
	}

	PipelineBuilder& PipelineBuilder::VertexBinding(uint32_t binding, uint32_t stride)
	{
		mVertexBindingDescriptions.emplace_back(binding, stride);

		return *this;
	}

	PipelineBuilder& PipelineBuilder::AttributeBinding(uint32_t location, uint32_t binding, vk::Format format,
		uint32_t offset)
	{
		mVertexAttributeDescriptions.emplace_back(location, binding, format, offset);

		return *this;
	}

	PipelineBuilder& PipelineBuilder::Shader(const vk::ShaderStageFlagBits stage, const ShaderModule& shader,
		const char* entryPoint, const vk::SpecializationInfo* specializationInfo)
	{
		vk::PipelineShaderStageCreateInfo info = { {}, stage, shader.Module(), entryPoint, specializationInfo };

		mModules.emplace_back(info);
		return *this;
	}

	PipelineBuilder& PipelineBuilder::Subpass(const uint32_t subpass)
	{ mSubpass = subpass; return *this; }

	PipelineBuilder& PipelineBuilder::InputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& value)
	{
		mInputAssemblyState = value; return *this;
	}

	PipelineBuilder& PipelineBuilder::RasterizationState(const vk::PipelineRasterizationStateCreateInfo& value)
	{
		mRasterizationState = value; return *this;
	}

	PipelineBuilder& PipelineBuilder::MultisampleState(const vk::PipelineMultisampleStateCreateInfo& value)
	{ mMultisampleState = value; return *this; }

	PipelineBuilder& PipelineBuilder::DepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& value)
	{
		mDepthStencilState = value; return *this;
	}

	PipelineBuilder& PipelineBuilder::DynamicState(const vk::DynamicState value)
	{
		mDynamicState.push_back(value); return *this;
	}

	vk::UniquePipeline PipelineBuilder::CreateUnique(const vk::Device& device, const vk::PipelineCache& pipelineCache,
		const vk::PipelineLayout& pipelineLayout, const vk::RenderPass& renderPass, bool defaultBlend)
	{
		// Add default colour blend attachment if necessary.
		if (mColorBlendAttachments.empty() && defaultBlend) {
			vk::PipelineColorBlendAttachmentState blend{};
			blend.blendEnable = 0;
			blend.srcColorBlendFactor = vk::BlendFactor::eOne;
			blend.dstColorBlendFactor = vk::BlendFactor::eZero;
			blend.colorBlendOp = vk::BlendOp::eAdd;
			blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			blend.alphaBlendOp = vk::BlendOp::eAdd;
			typedef vk::ColorComponentFlagBits ccbf;
			blend.colorWriteMask = ccbf::eR | ccbf::eG | ccbf::eB | ccbf::eA;
			mColorBlendAttachments.push_back(blend);
		}

		const auto count = static_cast<uint32_t>(mColorBlendAttachments.size());
		mColorBlendState.attachmentCount = count;
		mColorBlendState.pAttachments = count ? mColorBlendAttachments.data() : nullptr;

		const vk::PipelineViewportStateCreateInfo viewportState{
			{}, static_cast<uint32_t>(mViewports.size()), mViewports.data(), static_cast<uint32_t>(mScissors.size()), mScissors.data() };

		vk::PipelineVertexInputStateCreateInfo vertexInputState;
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(mVertexAttributeDescriptions.size());
		vertexInputState.pVertexAttributeDescriptions = mVertexAttributeDescriptions.data();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(mVertexBindingDescriptions.size());
		vertexInputState.pVertexBindingDescriptions = mVertexBindingDescriptions.data();

		const vk::PipelineDynamicStateCreateInfo dynState{ {}, static_cast<uint32_t>(mDynamicState.size()), mDynamicState.data() };

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.stageCount = static_cast<uint32_t>(mModules.size());
		pipelineInfo.pStages = mModules.data();
		pipelineInfo.pInputAssemblyState = &mInputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &mRasterizationState;
		pipelineInfo.pMultisampleState = &mMultisampleState;
		pipelineInfo.pColorBlendState = &mColorBlendState;
		pipelineInfo.pDepthStencilState = &mDepthStencilState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.pDynamicState = mDynamicState.empty() ? nullptr : &dynState;
		pipelineInfo.subpass = mSubpass;
		pipelineInfo.pTessellationState = &mTessellationState;

		// Setup chain of pNext structs
		if (mPNextChain.size() > 0)
		{
			for (size_t i = 0; i < mPNextChain.size() - 1; i++)
			{
				mPNextChain.at(i)->pNext = mPNextChain.at(i + 1);
			}

			pipelineInfo.pNext = mPNextChain.at(0);
		}

		auto [result, pipeline] = device.createGraphicsPipelineUnique(pipelineCache, pipelineInfo);
		// PUFFIN_TODO check result for vk::Result::ePipelineCompileRequiredEXT
		return std::move(pipeline);
	}
}
