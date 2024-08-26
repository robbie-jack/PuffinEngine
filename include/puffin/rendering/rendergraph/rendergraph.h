#pragma once

#include <string>
#include <unordered_map>

#include "puffin/rendering/rendergraph/renderpasstype.h"

namespace puffin::rendering
{
	class RenderPass;

	class RenderGraph
	{
	public:

		virtual ~RenderGraph() = 0;

		RenderPass* AddRenderPass(const std::string& name, RenderPassType renderPassType);

	protected:

		virtual RenderPass* AddRenderPassInternal(const std::string& name, RenderPassType renderPassType) = 0;

	private:

		std::unordered_map<std::string, RenderPass*> mRenderPasses;

	};
}
