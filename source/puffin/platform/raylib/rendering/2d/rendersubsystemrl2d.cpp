#include "puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.h"

#include <Camera2D.hpp>
#include <Color.hpp>

#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/rendering/2d/spritecomponent2d.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/platform/raylib/window/windowsubsystemrl.h"
#include "puffin/scene/scenegraphsubsystem.h"

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

		{
			ClearBackground({0, 178, 230});

			raylib::Camera2D camera;
			camera.SetTarget({ 0.f, 0.f });
			camera.SetOffset({ 1920 / 2.f, 1080 / 2.f });
			camera.SetRotation(0.f);
			camera.SetZoom(1.f);

			camera.BeginMode();

			{
				const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
				const auto registry = enttSubsystem->GetRegistry();

				const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

				const auto& spriteView = registry->view<const TransformComponent2D, const SpriteComponent2D>();

				for (auto& [entity, transform, sprite] : spriteView.each())
				{
					const auto id = enttSubsystem->GetID(entity);
					auto* node = sceneGraph->GetNode(id);

					raylib::Color colour(std::round(sprite.colour.x * 255),
						std::round(sprite.colour.y * 255),
						std::round(sprite.colour.z * 255));

					colour.DrawRectangle(transform.position.x, transform.position.y,
						32, 32);
				}
			}

			camera.EndMode();
		}

		EndDrawing();

		SwapScreenBuffer();

		mFrameCount++;
	}
}
