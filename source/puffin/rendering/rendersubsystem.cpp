#include "puffin/rendering/rendersubsystem.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(const std::shared_ptr<core::Engine>& engine)
		: Subsystem(engine)
	{
	}

	core::SubsystemType RenderSubsystem::GetType() const
	{
		return core::SubsystemType::Render;
	}

	RenderGraph* RenderSubsystem::GetRenderGraph()
	{
		return mRenderGraph;
	}
}
