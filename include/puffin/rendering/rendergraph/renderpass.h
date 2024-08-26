#pragma once

#include <string>

#include "renderpasstype.h"

namespace puffin::rendering
{
	class RenderPass
	{
	public:

		explicit RenderPass(std::string name, RenderPassType renderPassType);
		virtual ~RenderPass() = default;

	protected:

		std::string mName;
		RenderPassType mType;

	private:

		

	};
}
