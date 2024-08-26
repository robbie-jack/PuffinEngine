#pragma once

#include <string>
#include <unordered_map>

#include "puffin/rendering/rendergraph/renderpasstype.h"
#include "puffin/rendering/rendergraph/renderpass.h"

namespace puffin::rendering
{
	class RenderGraph
	{
	public:

		virtual ~RenderGraph() = default;

		RenderPass& AddRenderPass(const std::string& name, RenderPassType renderPassType);

		bool IsRenderPassValid(const std::string& name);
		RenderPass& GetRenderPass(const std::string& name);

		void Reset();

	protected:

		

	private:

		std::unordered_map<std::string, RenderPass> mRenderPasses;

	};
}
