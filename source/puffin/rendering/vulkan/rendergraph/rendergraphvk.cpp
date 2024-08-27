#include "puffin/rendering/vulkan/rendergraph/rendergraphvk.h"

#include <cassert>

#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	RenderPassVK& RenderGraphVK::AddRenderPass(const std::string& name, RenderPassType renderPassType)
	{
		mRenderPasses.emplace(name, RenderPassVK{ name, renderPassType });

		return mRenderPasses.at(name);
	}

	bool RenderGraphVK::IsRenderPassValid(const std::string& name)
	{
		return mRenderPasses.find(name) != mRenderPasses.end();
	}

	RenderPassVK& RenderGraphVK::GetRenderPass(const std::string& name)
	{
		assert(mRenderPasses.find(name) != mRenderPasses.end() && " RenderGraph::GetRenderPass");

		return mRenderPasses.at(name);
	}

	void RenderGraphVK::Reset()
	{
		mRenderPasses.clear();
	}
}
