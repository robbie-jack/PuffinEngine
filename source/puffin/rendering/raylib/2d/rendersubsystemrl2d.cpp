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
	}

	double RenderSubsystemRL2D::WaitForLastPresentationAndSampleTime()
	{
		return RenderSubsystem::WaitForLastPresentationAndSampleTime();
	}

	void RenderSubsystemRL2D::Render(double deltaTime)
	{
		RenderSubsystem::Render(deltaTime);

		SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
		//--------------------------------------------------------------------------------------

		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();
		auto* window = windowSubsystem->GetWindow();

		// Main game loop
		BeginDrawing();

		ClearBackground(RAYWHITE);

		DrawText("Congrats! You created your first raylib-cpp window!", 160, 200, 20, LIGHTGRAY);

		EndDrawing();
	}
}
