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

		/*
		 * Called each frame to wait for last presentation to complete and sample frame time
		 */
		virtual double WaitForLastPresentationAndSampleTime();

		/*
		 * Called each frame to render 2d/3d scene to display
		 */
		virtual void Render(double deltaTime);

		[[nodiscard]] core::SubsystemType GetType() const override;

		RenderGraph* GetRenderGraph();

	protected:

		RenderGraph* mRenderGraph;

	};
}