#pragma once

#include "puffin/rendering/renderer/renderer.h"

namespace puffin::rendering
{
	class ForwardRenderer3D : public Renderer
	{
	public:

		ForwardRenderer3D() = default;
		~ForwardRenderer3D() override = default;

		void Initialize() override;
		void Deinitialize() override;

	};
}