#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	RenderPassVK::RenderPassVK(std::string name, RenderPassType renderPassType)
		: mName(std::move(name)), mType(renderPassType)
	{

	}
}
