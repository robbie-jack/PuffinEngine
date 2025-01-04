#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	RenderPassVK::RenderPassVK(std::string name, RenderPassType renderPassType)
		: mName(std::move(name)), mType(renderPassType)
	{

	}

	void RenderPassVK::AddInputColorAttachment(const std::string& name)
	{
		mInputAttachments.push_back(name);
	}

	void RenderPassVK::SetInputDepthStencilAttachment(const std::string& name)
	{
		mInputDepthStencilAttachment = name;
	}

	void RenderPassVK::AddOutputColorAttachment(const std::string& name)
	{
		mOutputAttachments.push_back(name);
	}

	void RenderPassVK::SetOutputDepthStencilAttachment(const std::string& name)
	{
		mOutputDepthStencilAttachment = name;
	}

	void RenderPassVK::AddRequiredPass(const std::string& name)
	{
		mRequiredPasses.emplace(name);
	}

	void RenderPassVK::SetRecordCommandsCallback(std::function<void(const RenderPassVK&, vk::CommandBuffer&)> callback)
	{
		mRecordCommandsCallback = std::move(callback);
	}

	void RenderPassVK::ExecuteRecordCommandsCallback(vk::CommandBuffer& cmd) const
	{
		mRecordCommandsCallback(*this, cmd);
	}

	const std::string& RenderPassVK::GetName() const
	{
		return mName;
	}

	RenderPassType RenderPassVK::GetType() const
	{
		return mType;
	}

	const std::vector<std::string>& RenderPassVK::GetInputAttachments() const
	{
		return mInputAttachments;
	}

	const std::string& RenderPassVK::GetInputDepthStencilAttachment() const
	{
		return mInputDepthStencilAttachment;
	}

	const std::vector<std::string>& RenderPassVK::GetOutputAttachments() const
	{
		return mOutputAttachments;
	}

	const std::vector<std::string>& RenderPassVK::GetOutputBuffers() const
	{
		return mOutputBuffers;
	}

	const std::string& RenderPassVK::GetOutputDepthStencilAttachment() const
	{
		return mOutputDepthStencilAttachment;
	}

	const std::set<std::string>& RenderPassVK::GetRequiredPasses() const
	{
		return mRequiredPasses;
	}
}
