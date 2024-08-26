#pragma once

#include <unordered_map>
#include <string>

#include "puffin/rendering/rendergraph/rendergraph.h"

namespace puffin::rendering
{
	class RenderPassVK;

	class RenderGraphVK final : public RenderGraph
	{
	public:

		~RenderGraphVK() override = default;

	protected:

		RenderPass* AddRenderPassInternal(const std::string& name, RenderPassType renderPassType) override;

	};
}