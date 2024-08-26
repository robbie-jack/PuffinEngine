#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	RenderPass::RenderPass(std::string name, RenderPassType renderPassType)
		: mName(std::move(name)), mType(renderPassType)
	{

	}

	void RenderPass::BeginRendering()
	{
		mCommands.push_back(new RenderCommand(RenderCommandType::BeginRendering));
	}

	void RenderPass::EndRendering()
	{
		mCommands.push_back(new RenderCommand(RenderCommandType::EndRendering));
	}
}
