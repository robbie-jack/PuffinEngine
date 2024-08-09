#pragma once

#include "puffin/core/engine_subsystem.h"

namespace puffin::rendering
{
	/*
	 * Engine subsystem child which can execute render methods each frame
	 * for 2d/3d rendering
	 */
	class RenderSubsystem : public core::EngineSubsystem
	{
	public:

		RenderSubsystem(std::shared_ptr<core::Engine> engine);
		~RenderSubsystem() override = default;

		/*
		 * Called each frame to wait for last presentation to complete and sample frame time
		 */
		virtual void wait_for_last_presentation_and_sample_time();

		/*
		 * Called each frame to render 2d/3d scene to display
		 */
		virtual void render(double delta_time);

	};
}