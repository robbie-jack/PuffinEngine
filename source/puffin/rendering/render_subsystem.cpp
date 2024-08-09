#include "puffin/rendering/render_subsystem.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(std::shared_ptr<core::Engine> engine) : EngineSubsystem(engine)
	{
	}

	void RenderSubsystem::wait_for_last_presentation_and_sample_time()
	{

	}

	void RenderSubsystem::render(double delta_time)
	{

	}
}