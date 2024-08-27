#pragma once

#include <functional>
#include <string>
#include <vector>

#include "puffin/rendering/renderpasstype.h"
#include "puffin/rendering/vulkan/rendergraph/imagedescvk.h"
#include "puffin/rendering/vulkan/rendergraph/bufferdescvk.h"

namespace puffin::rendering
{
	class RenderPassVK
	{
	public:

		explicit RenderPassVK(std::string name, RenderPassType renderPassType);
		~RenderPassVK() = default;

		void AddInputAttachment(const std::string& name);
		void SetDepthStencilInputAttachment(const std::string& name);

		void AddOutputAttachment(const std::string& name, const ImageDescVK& attachmentDesc);
		void SetDepthStencilOutputAttachment(const std::string& name, const ImageDescVK& attachmentDesc);

		void SetRecordCommandsCallback(std::function<void(vk::CommandBuffer&)> callback);

		[[nodiscard]] const std::string& GetName() const;
		[[nodiscard]] RenderPassType GetType() const;

	private:

		std::string mName;
		RenderPassType mType;

		std::vector<std::string> mInputAttachment;
		std::vector<std::string> mInputBuffers;
		std::string mDepthStencilInputAttachment;

		std::vector<std::pair<std::string, ImageDescVK>> mOutputAttachments;
		std::vector<std::pair<std::string, BufferDescVK>> mOutputBuffers;
		std::pair<std::string, ImageDescVK> mDepthStencilOutputAttachment;

		std::function<void(vk::CommandBuffer&)> mRecordCommandsCallback;

	};
}
