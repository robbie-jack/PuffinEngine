#include "puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.h"

#include <Camera2D.hpp>
#include <Color.hpp>

#include "puffin/mathhelpers.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/rendering/2d/spritecomponent2d.h"
#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/nodes/transformnode2d.h"
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

		const double framerateLimitTime = 1.0 / static_cast<double>(mEngine->GetFramerateLimit());
		const double waitTime = framerateLimitTime - currentFrameTime;

		if (waitTime > 0.0)
		{
			WaitTime(waitTime);
		}

		return ::GetTime();
	}

	void RenderSubsystemRL2D::Render(double deltaTime)
	{
		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();

		BeginDrawing();

		// Sprite Rendering
		{
			ClearBackground({0, 178, 230});

			Vector2i windowSize = windowSubsystem->GetPrimaryWindowSize();

			raylib::Camera2D camera;
			camera.SetTarget({ 0.f, 0.f });
			camera.SetOffset({ static_cast<float>(windowSize.x) / 2.f, 
				static_cast<float>(windowSize.y) / 2.f });
			camera.SetRotation(0.f);
			camera.SetZoom(2.f);

			camera.BeginMode();

			{
				// Calculate t value for rendering interpolated position
				const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

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

#ifdef PFN_DOUBLE_PRECISION
					Vector2d position = { 0.0 };
#else
					Vector2f position{ 0.0f };
#endif

					Vector2f scale{ 1.f };

					if (const auto* transformNode = dynamic_cast<TransformNode2D*>(node); transformNode)
					{
						position = transformNode->GetGlobalTransform().position;
						scale = transformNode->GetGlobalTransform().scale;
					}
					else
					{
						position = transform.position;
						scale = transform.scale;
					}

					if (renderSettings.physicsInterpolationEnable)
					{
						physics::VelocityComponent2D velocity;

						if (registry->any_of<physics::VelocityComponent2D>(entity))
						{
							velocity = registry->get<physics::VelocityComponent2D>(entity);
						}
						else if (node)
						{
							if (const auto* parentNode = node->GetParent(); parentNode && parentNode->HasComponent<physics::VelocityComponent2D>())
							{
								velocity = parentNode->GetComponent<physics::VelocityComponent2D>();
							}
						}
						else
						{
							break;
						}

#ifdef PFN_DOUBLE_PRECISION
						Vector2d nextPosition = { 0.0 };
#else
						Vector2f nextPosition{ 0.0f };
#endif

						nextPosition = position + velocity.linear * mEngine->GetTimeStepFixed();

						position = maths::Lerp(position, nextPosition, t);
					}

					colour.DrawRectangle(static_cast<int>(std::round(position.x + sprite.offset.x)), static_cast<int>(std::round(position.y + sprite.offset.y)),
						static_cast<int>(std::round(scale.x)), static_cast<int>(std::round(scale.y)));
				}
			}

			camera.EndMode();
		}

		// Debug Drawing
		{
			
			// FPS
			{
				Color color = LIME; // Good FPS

				auto fps = static_cast<int>(std::round(1.0 / deltaTime));

				if ((fps < 30) && (fps >= 15)) color = ORANGE;  // Warning FPS
				else if (fps < 15) color = RED;             // Low FPS

				DrawText(TextFormat("%2i FPS", fps), 10, 10, 20, color);
			}
			
		}

		EndDrawing();

		SwapScreenBuffer();

		mFrameCount++;
	}
}
