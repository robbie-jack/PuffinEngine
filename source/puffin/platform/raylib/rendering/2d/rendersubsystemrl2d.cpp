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
#include "puffin/utility/benchmark.h"

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
		const auto framerateLimit = mEngine->GetFramerateLimit();

		if (framerateLimit > 0)
		{
			const double deltaTime = GetTime() - mEngine->GetLastTime();
			const double targetTime = 1.0 / static_cast<double>(framerateLimit);

			if (deltaTime < targetTime)
			{
				::WaitTime(targetTime - deltaTime);
			}
		}

		return GetTime();
	}

	void RenderSubsystemRL2D::Render(double deltaTime)
	{
		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();

		BeginDrawing();

		// Sprite Rendering
		{
			ClearBackground({0, 178, 230});

			Size windowSize = windowSubsystem->GetPrimaryWindowSize();

			raylib::Camera2D camera;
			camera.SetTarget({ 0.f, 0.f });
			camera.SetOffset({ static_cast<float>(windowSize.width) / 2.f, 
				static_cast<float>(windowSize.height) / 2.f });
			camera.SetRotation(0.f);
			camera.SetZoom(2.f);

			camera.BeginMode();

			DrawSprites();

			camera.EndMode();
		}

		// Debug Drawing
		{
			
			DebugDrawStats(deltaTime);
			
		}

		EndDrawing();

		SwapScreenBuffer();

		mFrameCount++;
	}

	void RenderSubsystemRL2D::WindowResized(Size size)
	{
		// PFN_TODO_RENDERING - Implement when adding viewport and render resolution scaling
	}

	void RenderSubsystemRL2D::ViewportResized(Size size)
	{
		// PFN_TODO_RENDERING - Implement when adding viewport and render resolution scaling
	}

	void RenderSubsystemRL2D::DrawSprites()
	{
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

				if (mRenderSettings.physicsInterpolationEnable)
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
	}

	void RenderSubsystemRL2D::DebugDrawStats(double deltaTime) const
	{
#define FPS_CAPTURE_FRAMES_COUNT 60
#define FPS_AVERAGE_TIME_SECONDS   1.f     // 1000 milliseconds
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)
		// FPS / Frametime
		{
			static int deltaTimeIdx = 0;
			static double deltaTimeHistory[FPS_CAPTURE_FRAMES_COUNT] = { 0.f };
			static double deltaTimeAvg = 0.f, timeLast = 0.f;

			if (mFrameCount == 0)
			{
				deltaTimeAvg = 0.f;
				timeLast = 0.f;
				deltaTimeIdx = 0;

				for (int i = 0; i < FPS_CAPTURE_FRAMES_COUNT; ++i) deltaTimeHistory[i] = 0;
			}

			if (GetTime() - timeLast > FPS_STEP)
			{
				timeLast = GetTime();
				deltaTimeIdx = (deltaTimeIdx + 1) % FPS_CAPTURE_FRAMES_COUNT;
				deltaTimeAvg -= deltaTimeHistory[deltaTimeIdx];
				deltaTimeHistory[deltaTimeIdx] = deltaTime / FPS_CAPTURE_FRAMES_COUNT;
				deltaTimeAvg += deltaTimeHistory[deltaTimeIdx];
			}

			auto fps = static_cast<int>(std::round(1.0 / deltaTimeAvg));

			Color color = LIME; // Good FPS
			if ((fps < 60) && (fps >= 30)) color = ORANGE;  // Warning FPS
			else if (fps < 30) color = RED;             // Low FPS

			DrawText(TextFormat("FPS: %2i, Frametime: %.3f ms", fps, deltaTimeAvg * 1000.f), 10, 10, 20, color);
		}

		// Benchmarks
		{
			auto* benchmarkManager = utility::BenchmarkManager::Get();

			int posY = 35, posYOffset = 25;
			for (auto& [name, benchmark] : benchmarkManager->GetBenchmarks())
			{
				DebugDrawBenchmark(benchmark, 10, posY);

				posY += posYOffset;
			}
		}
	}

	void RenderSubsystemRL2D::DebugDrawBenchmark(const utility::Benchmark& benchmark, int posX, int posY) const
	{
		DrawText(TextFormat("%s Frametime: %.3f ms", benchmark.GetData().name.c_str(), benchmark.GetData().timeElapsed * 1000.f), posX, posY, 20, WHITE);
	}
}
