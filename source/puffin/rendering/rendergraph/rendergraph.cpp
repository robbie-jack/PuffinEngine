#include "puffin/rendering/rendergraph/rendergraph.h"

namespace puffin::rendering
{
	RenderPass* RenderGraph::AddRenderPass(const std::string& name, RenderPassType renderPassType)
	{
		auto renderPass = AddRenderPassInternal(name, renderPassType);

		mRenderPasses.emplace(name, renderPass);

		return renderPass;
	}
}
