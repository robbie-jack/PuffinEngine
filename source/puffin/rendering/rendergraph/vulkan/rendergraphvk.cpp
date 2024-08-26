#include "puffin/rendering/rendergraph/vulkan/rendergraphvk.h"

#include "puffin/rendering/rendergraph/vulkan/renderpassvk.h"

namespace puffin::rendering
{
	RenderPass* RenderGraphVK::AddRenderPassInternal(const std::string& name, RenderPassType renderPassType)
	{
		return new RenderPassVK(name, renderPassType);
	}
}
