#include "puffin/rendering/rendersubsystem.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(const std::shared_ptr<core::Engine>& engine)
		: Subsystem(engine)
	{
	}

	double RenderSubsystem::WaitForLastPresentationAndSampleTime()
	{
		return 0.0;
	}

	void RenderSubsystem::Render(double deltaTime)
	{

	}

	core::SubsystemType RenderSubsystem::GetType() const
	{
		return core::SubsystemType::Render;
	}

	RenderGraph& RenderSubsystem::GetRenderGraph()
	{
		return mRenderGraph;
	}
}
