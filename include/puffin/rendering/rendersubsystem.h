#pragma once

#include "puffin/core/subsystem.h"

namespace puffin::rendering
{
	class RenderGraph;

	/*
	 * A Render Subsystem implements a rendering api (i.e Vulkan, DirectX, etc...) and
	 * handles common rendering functionality like resource management for that api
	 * Explicit rendering functionality like for a forward render pass, is handled in the renderer classes
	 */
	class RenderSubsystem : public core::Subsystem
	{
	public:

		explicit RenderSubsystem(const std::shared_ptr<core::Engine>& engine);
		~RenderSubsystem() override = default;

		[[nodiscard]] core::SubsystemType GetType() const override;

		RenderGraph* GetRenderGraph();

	protected:

		RenderGraph* mRenderGraph;

	};
}