#pragma once

#include "puffin/rendering/renderer/renderer.h"

namespace puffin::rendering
{
	class ForwardRenderer3D : public Renderer
	{
	public:

		explicit ForwardRenderer3D(RenderSubsystem* renderSubsystem);
		~ForwardRenderer3D() override = default;

		void Initialize() override;
		void Deinitialize() override;

		void SetupRenderPasses() override;
		void PrepareRenderCommands() override;

	};
}