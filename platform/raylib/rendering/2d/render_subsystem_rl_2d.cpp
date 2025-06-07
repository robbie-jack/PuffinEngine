#include "raylib/rendering/2d/render_subsystem_rl_2d.h"

#include <Camera2D.hpp>
#include <Color.hpp>

#include "puffin/mathhelpers.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/rendering/2d/spritecomponent2d.h"
#include "puffin/components/rendering/2d/cameracomponent2d.h"
#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/nodes/transformnode2d.h"
#include "raylib/window/window_subsystem_rl.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/utility/benchmark.h"

#define FPS_CAPTURE_FRAMES_COUNT 60
#define FPS_AVERAGE_TIME_SECONDS   1.f     // 1000 milliseconds
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)

static int deltaTimeIdx = 0;
static bool updateAvg = false;
static std::unordered_map<std::string, double[FPS_CAPTURE_FRAMES_COUNT]> benchmarkHistory;
static std::unordered_map<std::string, double> benchmarkAvg;

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

		auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

		InitSettingsAndSignals();
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
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		UpdateCamera();

		BeginDrawing();

		// Sprite Rendering
		{
			ClearBackground({0, 178, 230});

			mCamera.BeginMode();

			DrawSprites();

			mCamera.EndMode();
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

	void RenderSubsystemRL2D::InitSettingsAndSignals()
	{
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

		// Pixel Scale
		{
			mPixelScale = settingsManager->Get<int32_t>("rendering", "pixel_scale").value_or(0);

			signalSubsystem->GetOrCreateSignal("rendering_pixel_scale")->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mPixelScale = settingsManager->Get<int32_t>("rendering", "pixel_scale").value_or(0);
			}));
		}
	}

	void RenderSubsystemRL2D::UpdateCamera()
	{
		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();
		auto cameraSubsystem = mEngine->GetSubsystem<rendering::CameraSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		Size windowSize = windowSubsystem->GetPrimaryWindowSize();

		auto activeCamID = cameraSubsystem->GetActiveCameraID();
		auto activeCamEntity = enttSubsystem->GetEntity(activeCamID);

		auto activeCamTransform = registry->get<TransformComponent2D>(activeCamEntity);
		auto activeCamCamera = registry->get<CameraComponent2D>(activeCamEntity);

		mCamera.SetTarget({ activeCamTransform.position.x * mPixelScale, activeCamTransform.position.y * mPixelScale });
		mCamera.SetOffset({ static_cast<float>(windowSize.width) / 2.f,
			static_cast<float>(windowSize.height) / 2.f });
		mCamera.SetRotation(activeCamCamera.rotation);
		mCamera.SetZoom(activeCamCamera.zoom);
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

				raylib::Vector2 scaledPos = ScaleWorldToPixel({ position.x, position.y });
				raylib::Vector2 scaledOffset = ScaleWorldToPixel({ sprite.offset.x, sprite.offset.y });

				int32_t pixelWidth = static_cast<int>(std::round(ScaleWorldToPixel(scale.x)));
				int32_t pixelHeight = static_cast<int>(std::round(ScaleWorldToPixel(scale.y)));

				colour.DrawRectangle(scaledPos.x + scaledOffset.x, scaledPos.y + scaledOffset.y, pixelWidth, pixelHeight);
			}
		}
	}

	void RenderSubsystemRL2D::DebugDrawStats(double deltaTime) const
	{
		// FPS / Frametime
		{
			static double deltaTimeHistory[FPS_CAPTURE_FRAMES_COUNT] = { 0.f };
			static double deltaTimeAvg = 0.f, timeLast = 0.f;

			if (mFrameCount == 0)
			{
				deltaTimeAvg = 0.f;
				timeLast = 0.f;
				deltaTimeIdx = 0;

				for (int i = 0; i < FPS_CAPTURE_FRAMES_COUNT; ++i) deltaTimeHistory[i] = 0;

				benchmarkHistory.clear();
				benchmarkAvg.clear();
			}

			if (GetTime() - timeLast > FPS_STEP)
			{
				timeLast = GetTime();
				deltaTimeIdx = (deltaTimeIdx + 1) % FPS_CAPTURE_FRAMES_COUNT;
				deltaTimeAvg -= deltaTimeHistory[deltaTimeIdx];
				deltaTimeHistory[deltaTimeIdx] = deltaTime / FPS_CAPTURE_FRAMES_COUNT;
				deltaTimeAvg += deltaTimeHistory[deltaTimeIdx];

				updateAvg = true;
			}

			auto fps = static_cast<int>(std::round(1.0 / deltaTimeAvg));

			Color color = LIME; // Good FPS
			if ((fps < 60) && (fps >= 30)) color = ORANGE;  // Warning FPS
			else if (fps < 30) color = RED;             // Low FPS

			DrawText(TextFormat("FPS: %2i, Frametime: %.3f ms", fps, deltaTimeAvg * 1000.f), 10, 10, 20, color);
		}

		// Benchmarks
		{
			const std::vector<std::string> benchmarkNames =
			{
				"Input",
				"WaitForLastPresentationAndSample",
				"EngineUpdate",
				"FixedUpdate",
				"Update",
				"Render"
			};

			auto* benchmarkManager = utility::BenchmarkManager::Get();

			int posY = 35, posYOffset = 25;
			for (auto& benchmarkName : benchmarkNames)
			{
				auto* benchmark = benchmarkManager->Get(benchmarkName);

				if (!benchmark)
					continue;

				DebugDrawBenchmark(benchmark, 10, posY);

				posY += posYOffset;
			}

			updateAvg = false;
		}
	}

	void RenderSubsystemRL2D::DebugDrawBenchmark(const utility::Benchmark* benchmark, int posX, int& posY) const
	{
		const std::string& name = benchmark->GetData().name;

		if (benchmarkHistory.find(name) == benchmarkHistory.end())
		{
			benchmarkHistory.emplace();

			for (int i = 0; i < FPS_CAPTURE_FRAMES_COUNT; ++i) benchmarkHistory[name][i] = 0;
		}

		if (benchmarkAvg.find(name) == benchmarkAvg.end())
		{
			benchmarkAvg.emplace();

			benchmarkAvg[name] = 0;
		}

		if (updateAvg)
		{
			benchmarkAvg[name] -= benchmarkHistory[name][deltaTimeIdx];
			benchmarkHistory[name][deltaTimeIdx] = benchmark->GetData().timeElapsed / FPS_CAPTURE_FRAMES_COUNT;
			benchmarkAvg[name] += benchmarkHistory[name][deltaTimeIdx];
		}

		DrawText(TextFormat("%s Frametime: %.3f ms", benchmark->GetData().name.c_str(), benchmarkAvg[name] * 1000.f), posX, posY, 20, WHITE);

		for (auto& [childName, childBenchmark] : benchmark->GetBenchmarks())
		{
			posY += 25;

			DebugDrawBenchmark(&childBenchmark, posX + 20, posY);
		}
	}

	float RenderSubsystemRL2D::ScaleWorldToPixel(const float& val) const
	{
		return val * static_cast<float>(mPixelScale);
	}

	raylib::Vector2 RenderSubsystemRL2D::ScaleWorldToPixel(const raylib::Vector2& val) const
	{
		return val.Scale(static_cast<float>(mPixelScale));
	}
}
