#pragma once

#include "puffin/rendering/renderer/vulkan/renderervk.h"

namespace puffin::rendering
{
	class ForwardRendererVK : public RendererVK
	{
	public:

		ForwardRendererVK() = default;
		~ForwardRendererVK() override = default;

		void Initialize() override;
		void Deinitialize() override;

	};
}