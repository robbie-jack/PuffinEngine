#pragma once

#include <string>
#include <unordered_map>

#include "puffin/rendering/renderpasstype.h"
#include "puffin/rendering/vulkan/rendergraph/renderpassvk.h"

namespace puffin::rendering
{
	class RenderGraphVK
	{
	public:

		virtual ~RenderGraphVK() = default;

		RenderPassVK& AddRenderPass(const std::string& name, RenderPassType renderPassType);

		bool IsRenderPassValid(const std::string& name);
		RenderPassVK& GetRenderPass(const std::string& name);

		void Reset();

	private:

		std::unordered_map<std::string, RenderPassVK> mRenderPasses;

	};
}
