#include "puffin/rendering/rendergraph/rendergraph.h"

#include <cassert>

#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	RenderPass& RenderGraph::AddRenderPass(const std::string& name, RenderPassType renderPassType)
	{
		mRenderPasses.emplace(name, RenderPass{ name, renderPassType });

		return mRenderPasses.at(name);
	}

	bool RenderGraph::IsRenderPassValid(const std::string& name)
	{
		return mRenderPasses.find(name) != mRenderPasses.end();
	}

	RenderPass& RenderGraph::GetRenderPass(const std::string& name)
	{
		assert(mRenderPasses.find(name) != mRenderPasses.end() && " RenderGraph::GetRenderPass");

		return mRenderPasses.at(name);
	}

	void RenderGraph::Reset()
	{
		mRenderPasses.clear();
	}
}
