#include "puffin/rendering/raylib/2d/rendersubsystemrl2d.h"

#include "puffin/core/engine.h"
#include "puffin/window/raylib/windowsubsystemrl.h"

namespace puffin::rendering
{
	RenderSubsystemRL2D::RenderSubsystemRL2D(const std::shared_ptr<core::Engine>& engine) : RenderSubsystem(engine)
	{
		
	}

	void RenderSubsystemRL2D::RegisterTypes()
	{
		mEngine->RegisterSubsystem<window::WindowSubsystemRL>();
	}

	double RenderSubsystemRL2D::WaitForLastPresentationAndSampleTime()
	{
		return RenderSubsystem::WaitForLastPresentationAndSampleTime();
	}

	void RenderSubsystemRL2D::Render(double deltaTime)
	{
		RenderSubsystem::Render(deltaTime);


	}
}
