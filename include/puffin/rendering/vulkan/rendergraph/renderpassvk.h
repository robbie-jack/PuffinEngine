#pragma once

#include <string>
#include <vector>

#include "puffin/rendering/renderpasstype.h"

namespace puffin::rendering
{
	class RenderPassVK
	{
	public:

		explicit RenderPassVK(std::string name, RenderPassType renderPassType);
		~RenderPassVK() = default;

	private:

		std::string mName;
		RenderPassType mType;

	};
}
