#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	RenderPass::RenderPass(std::string name, RenderPassType renderPassType)
		: mName(std::move(name)), mType(renderPassType)
	{

	}
}
