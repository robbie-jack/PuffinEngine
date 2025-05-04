#include "puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.h"

#include "puffin/core/engine.h"
#include "puffin/platform/raylib/window/windowsubsystemrl.h"

namespace puffin::rendering
{
	RenderSubsystemRL2D::RenderSubsystemRL2D(const std::shared_ptr<core::Engine>& engine) : RenderSubsystem(engine)
	{
		
	}

	void RenderSubsystemRL2D::RegisterTypes()
	{
	}

	void RenderSubsystemRL2D::Initialize(core::SubsystemManager* subsystemManager)
	{
		RenderSubsystem::Initialize(subsystemManager);
	}

	void RenderSubsystemRL2D::Deinitialize()
	{
		RenderSubsystem::Deinitialize();


	}

	double RenderSubsystemRL2D::WaitForLastPresentationAndSampleTime()
	{
		const double currentTime = ::GetTime();
		const double lastTime = mEngine->GetLastTime();

		const double currentFrameTime = currentTime - lastTime;

		const double waitTime = (1.0 / static_cast<double>(mEngine->GetFramerateLimit())) - currentFrameTime;

		if (waitTime > 0.0)
		{
			WaitTime(waitTime);
		}

		return ::GetTime();
	}

	void RenderSubsystemRL2D::Render(double deltaTime)
	{
		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();
		auto* window = windowSubsystem->GetWindow();

		// Main game loop
		BeginDrawing();

		ClearBackground(RAYWHITE);

		DrawText("Congrats! You created your first raylib-cpp window!", 160, 200, 20, LIGHTGRAY);

		EndDrawing();

		SwapScreenBuffer();

		mFrameCount++;
	}
}
