#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

#include "vulkan/vulkan.hpp"

#include "puffin/rendering/renderpasstype.h"

namespace puffin::rendering
{
	class RenderGraphVK;

	class RenderPassVK
	{
	public:

		explicit RenderPassVK(std::string name, RenderPassType renderPassType);
		~RenderPassVK() = default;

		void AddInputColorAttachment(const std::string& name);
		void SetInputDepthStencilAttachment(const std::string& name);

		void AddOutputColorAttachment(const std::string& name);
		void SetOutputDepthStencilAttachment(const std::string& name);

		void AddRequiredPass(const std::string& name);

		void SetRecordCommandsCallback(std::function<void(vk::CommandBuffer&)> callback);
		void ExecuteRecordCommandsCallback(vk::CommandBuffer& cmd) const;

		[[nodiscard]] const std::string& GetName() const;
		[[nodiscard]] RenderPassType GetType() const;

		[[nodiscard]] const std::vector<std::string>& GetInputAttachments() const;
		[[nodiscard]] const std::string& GetInputDepthStencilAttachment() const;

		[[nodiscard]] const std::vector<std::string>& GetOutputAttachments() const;
		[[nodiscard]] const std::vector<std::string>& GetOutputBuffers() const;
		[[nodiscard]] const std::string& GetOutputDepthStencilAttachment() const;

		[[nodiscard]] const std::set<std::string>& GetRequiredPasses() const;

	private:

		std::string mName;
		RenderPassType mType;

		std::vector<std::string> mInputAttachments;
		std::string mInputDepthStencilAttachment;

		std::vector<std::string> mOutputAttachments;
		std::vector<std::string> mOutputBuffers;
		std::string mOutputDepthStencilAttachment;

		/*
		 * Set of passes which must run before this one
		 */
		std::set<std::string> mRequiredPasses;

		std::function<void(vk::CommandBuffer&)> mRecordCommandsCallback;

	};
}
