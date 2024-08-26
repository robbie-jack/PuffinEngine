#pragma once

#include <string>
#include <vector>

#include "puffin/rendering/rendergraph/renderpasstype.h"
#include "puffin/rendering/rendergraph/rendercommand.h"

namespace puffin::rendering
{
	class RenderPass
	{
	public:

		explicit RenderPass(std::string name, RenderPassType renderPassType);
		virtual ~RenderPass() = default;

		void BeginRendering();
		void EndRendering();

	private:

		std::string mName;
		RenderPassType mType;

		std::vector<RenderCommand*> mCommands;

	};
}
