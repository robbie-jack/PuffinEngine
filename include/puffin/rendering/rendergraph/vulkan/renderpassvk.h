#pragma once

#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	class RenderPassVK final : public RenderPass
	{
	public:

		RenderPassVK(const std::string& name, RenderPassType renderPassType);
		~RenderPassVK() override = default;



	private:



	};
}