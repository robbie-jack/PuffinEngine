#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	RenderPassVK::RenderPassVK(std::string name, RenderPassType renderPassType)
		: mName(std::move(name)), mType(renderPassType)
	{

	}

	void RenderPassVK::AddInputAttachment(const std::string& name)
	{
		mInputAttachment.push_back(name);
	}

	void RenderPassVK::SetDepthStencilInputAttachment(const std::string& name)
	{
		mDepthStencilInputAttachment = name;
	}

	void RenderPassVK::AddOutputAttachment(const std::string& name, const ImageDescVK& attachmentDesc)
	{
		mOutputAttachments.emplace_back(name, attachmentDesc);
	}

	void RenderPassVK::SetDepthStencilOutputAttachment(const std::string& name, const ImageDescVK& attachmentDesc)
	{
		mDepthStencilOutputAttachment = { name, attachmentDesc };
	}

	void RenderPassVK::SetRecordCommandsCallback(std::function<void(vk::CommandBuffer&)> callback)
	{
		mRecordCommandsCallback = std::move(callback);
	}

	const std::string& RenderPassVK::GetName() const
	{
		return mName;
	}

	RenderPassType RenderPassVK::GetType() const
	{
		return mType;
	}
}
