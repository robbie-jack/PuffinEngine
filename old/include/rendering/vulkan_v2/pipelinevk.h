#pragma once

#include <vulkan/vulkan.hpp>

#include "puffin/rendering/vulkan/vertexvk.h"

namespace puffin::rendering::util
{
	class ShaderModule
	{
	public:

		ShaderModule() = default;
		ShaderModule(const vk::Device& device, const std::string& filename);
		ShaderModule(const vk::Device& device, const std::vector<uint32_t>& code);

		[[nodiscard]] bool Successful() const;
		[[nodiscard]] vk::ShaderModule Module() const;

	private:

		vk::UniqueShaderModule mModule;
		bool mSuccessful = false;

	};

	class PipelineLayoutBuilder
	{
	public:

		static PipelineLayoutBuilder Begin();

		PipelineLayoutBuilder& DescriptorSetLayout(vk::DescriptorSetLayout layout);
		PipelineLayoutBuilder& PushConstantRange(vk::PushConstantRange range);
		vk::UniquePipelineLayout CreateUnique(const vk::Device& device);

	private:

		std::vector<vk::DescriptorSetLayout> mSetLayouts;
		std::vector<vk::PushConstantRange> mPushRanges;

	};

	class PipelineBuilder
	{
	public:

		PipelineBuilder(uint32_t width, uint32_t height);

		static PipelineBuilder Begin(uint32_t width, uint32_t height);

		void Init();

		PipelineBuilder& Viewport(const vk::Viewport& viewport);

		PipelineBuilder& Scissor(const vk::Rect2D& scissor);

		PipelineBuilder& VertexLayout(const VertexLayout& vertexLayout);

		PipelineBuilder& VertexBinding(uint32_t binding, uint32_t stride);

		PipelineBuilder& AttributeBinding(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset);

		PipelineBuilder& Shader(const vk::ShaderStageFlagBits stage, const ShaderModule& shader,
		                        const char* entryPoint = "main", const vk::SpecializationInfo* specializationInfo = nullptr);

		PipelineBuilder& Subpass(const uint32_t subpass);

		// PUFFIN_TODO Add Methods for setting up color blend attachments & color blend state

		PipelineBuilder& InputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& value);

		PipelineBuilder& RasterizationState(const vk::PipelineRasterizationStateCreateInfo& value);

		PipelineBuilder& MultisampleState(const vk::PipelineMultisampleStateCreateInfo& value);

		PipelineBuilder& DepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& value);

		PipelineBuilder& DynamicState(const vk::DynamicState value);

		template<typename T>
		PipelineBuilder& AddPNext(T* structure)
		{
			mPNextChain.push_back(reinterpret_cast<vk::BaseOutStructure*>(structure));
			return *this;
		}

		vk::UniquePipeline CreateUnique(const vk::Device& device,
			const vk::PipelineCache& pipelineCache,
			const vk::PipelineLayout& pipelineLayout,
			const vk::RenderPass& renderPass, bool defaultBlend = true);

	private:

		std::vector<vk::Viewport> mViewports;
		std::vector<vk::Rect2D> mScissors;

		vk::PipelineInputAssemblyStateCreateInfo mInputAssemblyState;
		vk::PipelineRasterizationStateCreateInfo mRasterizationState;
		vk::PipelineMultisampleStateCreateInfo mMultisampleState;
		vk::PipelineDepthStencilStateCreateInfo mDepthStencilState;
		vk::PipelineColorBlendStateCreateInfo mColorBlendState;
		vk::PipelineTessellationStateCreateInfo mTessellationState;
		std::vector<vk::PipelineColorBlendAttachmentState> mColorBlendAttachments;
		std::vector<vk::PipelineShaderStageCreateInfo> mModules;
		//std::vector<std::unique_ptr<SpecData>> m_moduleSpecializations;
		std::vector<vk::VertexInputAttributeDescription> mVertexAttributeDescriptions;
		std::vector<vk::VertexInputBindingDescription> mVertexBindingDescriptions;
		std::vector<vk::DynamicState> mDynamicState;
		std::vector<vk::BaseOutStructure*> mPNextChain;
		uint32_t mSubpass = 0;
	};
}
