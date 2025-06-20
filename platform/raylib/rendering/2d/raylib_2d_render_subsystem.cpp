#include "raylib/rendering/2d/raylib_2d_render_subsystem.h"

#include <Camera2D.hpp>
#include <Color.hpp>

#include "math_helpers.h"
#include "component/transform_component_2d.h"
#include "component/physics/2d/velocity_component_2d.h"
#include "component/rendering/2d/sprite_component_2d.h"
#include "component/rendering/2d/camera_component_2d.h"
#include "core/engine.h"
#include "core/settings_manager.h"
#include "core/signal_subsystem.h"
#include "ecs/entt_subsystem.h"
#include "node/transform_2d_node.h"
#include "node/physics/2d/rigidbody_2d_node.h"
#include "node/rendering/2d/sprite_2d_node.h"
#include "raylib/window/raylib_window_subsystem.h"
#include "rendering/camera_subsystem.h"
#include "scene/scene_graph_subsystem.h"
#include "utility/benchmark.h"

#define FPS_CAPTURE_FRAMES_COUNT 60
#define FPS_AVERAGE_TIME_SECONDS   1.f     // 1000 milliseconds
#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)

static int deltaTimeIdx = 0;
static bool updateAvg = false;
static std::unordered_map<std::string, double[FPS_CAPTURE_FRAMES_COUNT]> benchmarkHistory;
static std::unordered_map<std::string, double> benchmarkAvg;

namespace puffin::rendering
{
	Raylib2DRenderSubsystem::Raylib2DRenderSubsystem(const std::shared_ptr<core::Engine>& engine) : RenderSubsystem(engine)
	{
		
	}

	void Raylib2DRenderSubsystem::RegisterTypes()
	{
	}

	void Raylib2DRenderSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		RenderSubsystem::Initialize(subsystemManager);

		auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		auto signalSubsystem = subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

		InitSettingsAndSignals();
	}

	void Raylib2DRenderSubsystem::Deinitialize()
	{
		RenderSubsystem::Deinitialize();
	}

	double Raylib2DRenderSubsystem::WaitForLastPresentationAndSampleTime()
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

	void Raylib2DRenderSubsystem::Render(double deltaTime)
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		UpdateCamera();

		BeginDrawing();

		// Sprite Rendering
		{
			ClearBackground({0, 178, 230});

			m_camera.BeginMode();

			DrawSprites();

			m_camera.EndMode();
		}

		// Draw Text
		{
			for (const auto& textDraw : m_textDraws)
			{
				DrawText(textDraw);
			}

			m_textDraws.clear();
		}

		// Debug Drawing
		{
			//DebugDrawStats(deltaTime);
		}

		EndDrawing();

		SwapScreenBuffer();

		mFrameCount++;
	}

	void Raylib2DRenderSubsystem::WindowResized(Size size)
	{
		// PFN_TODO_RENDERING - Implement when adding viewport and render resolution scaling
	}

	void Raylib2DRenderSubsystem::ViewportResized(Size size)
	{
		// PFN_TODO_RENDERING - Implement when adding viewport and render resolution scaling
	}

	void Raylib2DRenderSubsystem::DrawTextToScreen(const std::string& string, int posX, int posY, int fontSize, Vector3f color)
	{
		m_textDraws.emplace_back(string, posX, posY, fontSize, raylib::Color{ static_cast<unsigned char>(std::round(color.x * 255)), 
			static_cast<unsigned char>(std::round(color.y * 255)), static_cast<unsigned char>(std::round(color.z * 255)) });
	}

	void Raylib2DRenderSubsystem::InitSettingsAndSignals()
	{
		auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

		// Pixel Scale
		{
			m_pixelScale = settingsManager->Get<int32_t>("rendering", "pixel_scale").value_or(0);

			signalSubsystem->GetOrCreateSignal("rendering_pixel_scale")->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				m_pixelScale = settingsManager->Get<int32_t>("rendering", "pixel_scale").value_or(0);
			}));
		}
	}

	void Raylib2DRenderSubsystem::UpdateCamera()
	{
		auto* windowSubsystem = mEngine->GetSubsystem<window::RaylibWindowSubsystem>();
		auto cameraSubsystem = mEngine->GetSubsystem<rendering::CameraSubsystem>();
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		auto registry = enttSubsystem->GetRegistry();

		if (cameraSubsystem->IsActiveCameraValid())
			return;

		Size windowSize = windowSubsystem->GetPrimaryWindowSize();
		auto activeCamID = cameraSubsystem->GetActiveCameraID();
		auto activeCamEntity = enttSubsystem->GetEntity(activeCamID);

		auto activeCamTransform = registry->get<TransformComponent2D>(activeCamEntity);
		auto activeCamCamera = registry->get<CameraComponent2D>(activeCamEntity);

		m_camera.SetTarget({ activeCamTransform.position.x * m_pixelScale, activeCamTransform.position.y * m_pixelScale });
		m_camera.SetOffset({ static_cast<float>(windowSize.width) / 2.f,
			static_cast<float>(windowSize.height) / 2.f });
		m_camera.SetRotation(activeCamCamera.rotation);
		m_camera.SetZoom(activeCamCamera.zoom);
	}

	void Raylib2DRenderSubsystem::DrawSprites()
	{
		DrawSpriteNodes();
		DrawSpriteComponents();
	}

	void Raylib2DRenderSubsystem::DrawSpriteNodes() const
	{
		// Calculate t value for rendering interpolated position
		const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

		const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

		std::vector<Sprite2DNode*> sprites;
		sceneGraph->GetNodes(sprites);
		for (auto& sprite : sprites)
		{
			const auto& transform = sprite->GetGlobalTransform();

			raylib::Color colour(std::round(sprite->GetColour().x * 255),
				std::round(sprite->GetColour().y * 255),
				std::round(sprite->GetColour().z * 255));

#ifdef PFN_DOUBLE_PRECISION
			Vector2d position = { 0.0 };
#else
			Vector2f position{ 0.0f };
#endif

			Vector2f scale{ 1.f };

			position = transform.position;
			scale = transform.scale;

			auto* rigidbody = dynamic_cast<physics::Rigidbody2DNode*>(sprite->GetParent());
			if (mRenderSettings.physicsInterpolationEnable && rigidbody)
			{
#ifdef PFN_DOUBLE_PRECISION
				Vector2d nextPosition = { 0.0 };
#else
				Vector2f nextPosition{ 0.0f };
#endif

				nextPosition = position + rigidbody->GetLinearVelocity() * mEngine->GetTimeStepFixed();

				position = maths::Lerp(position, nextPosition, t);
			}

			raylib::Vector2 scaledPos = ScaleWorldToPixel({ position.x, position.y });
			raylib::Vector2 scaledOffset = ScaleWorldToPixel({ sprite->GetOffset().x, sprite->GetOffset().y });

			int32_t pixelWidth = static_cast<int>(std::round(ScaleWorldToPixel(scale.x)));
			int32_t pixelHeight = static_cast<int>(std::round(ScaleWorldToPixel(scale.y)));

			colour.DrawRectangle(scaledPos.x + scaledOffset.x, scaledPos.y + scaledOffset.y, pixelWidth, pixelHeight);
		}
	}

	void Raylib2DRenderSubsystem::DrawSpriteComponents() const
	{
		// Calculate t value for rendering interpolated position
		const double t = mEngine->GetAccumulatedTime() / mEngine->GetTimeStepFixed();

		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto& spriteView = registry->view<const TransformComponent2D, const SpriteComponent2D>();

		for (auto& [entity, transform, sprite] : spriteView.each())
		{
			raylib::Color colour(std::round(sprite.colour.x * 255),
				std::round(sprite.colour.y * 255),
				std::round(sprite.colour.z * 255));

#ifdef PFN_DOUBLE_PRECISION
			Vector2d position = { 0.0 };
#else
			Vector2f position{ 0.0f };
#endif

			Vector2f scale{ 1.f };

			position = transform.position;
			scale = transform.scale;

			if (mRenderSettings.physicsInterpolationEnable && registry->any_of<physics::VelocityComponent2D>(entity))
			{
				physics::VelocityComponent2D velocity = registry->get<physics::VelocityComponent2D>(entity);

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

	void Raylib2DRenderSubsystem::DrawText(const TextDraw& textDraw) const
	{
		::DrawText(textDraw.string.c_str(), textDraw.posX, textDraw.posY, textDraw.fontSize, textDraw.color);
	}

	void Raylib2DRenderSubsystem::DebugDrawStats(double deltaTime) const
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

			::DrawText(TextFormat("FPS: %2i, Frametime: %.3f ms", fps, deltaTimeAvg * 1000.f), 10, 10, 20, color);
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

	void Raylib2DRenderSubsystem::DebugDrawBenchmark(const utility::Benchmark* benchmark, int posX, int& posY) const
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

		::DrawText(TextFormat("%s Frametime: %.3f ms", benchmark->GetData().name.c_str(), benchmarkAvg[name] * 1000.f), posX, posY, 20, WHITE);

		for (auto& [childName, childBenchmark] : benchmark->GetBenchmarks())
		{
			posY += 25;

			DebugDrawBenchmark(&childBenchmark, posX + 20, posY);
		}
	}

	float Raylib2DRenderSubsystem::ScaleWorldToPixel(const float& val) const
	{
		return val * static_cast<float>(m_pixelScale);
	}

	raylib::Vector2 Raylib2DRenderSubsystem::ScaleWorldToPixel(const raylib::Vector2& val) const
	{
		return val.Scale(static_cast<float>(m_pixelScale));
	}
}
