#pragma once

#include "puffin/rendering/renderer/vulkan/renderervk.h"

namespace puffin::rendering
{
	class ForwardRenderer3DVK : public RendererVK
	{
	public:

		ForwardRenderer3DVK() = default;
		~ForwardRenderer3DVK() override = default;

		void Initialize() override;
		void Deinitialize() override;

	};
}