#pragma once

#include <Camera2D.hpp>

#include "rendering/render_subsystem.h"

namespace puffin
{
	namespace utility
	{
		class Benchmark;
	}
}

namespace puffin::rendering
{
	class Raylib2DRenderSubsystem : public RenderSubsystem
	{
	public:

		explicit Raylib2DRenderSubsystem(const std::shared_ptr<core::Engine>& engine);
		~Raylib2DRenderSubsystem() override = default;

		void RegisterTypes() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		double WaitForLastPresentationAndSampleTime() override;

		void Render(double deltaTime) override;

		void WindowResized(Size size) override;
		void ViewportResized(Size size) override;

	protected:

		void InitSettingsAndSignals();

		void UpdateCamera();

		void DrawSprites();
		void DrawSpriteNodes() const;
		void DrawSpriteComponents() const;
		void DebugDrawStats(double deltaTime) const;
		void DebugDrawBenchmark(const utility::Benchmark* benchmark, int posX, int& posY) const;

		[[nodiscard]] float ScaleWorldToPixel(const float& val) const;
		[[nodiscard]] raylib::Vector2 ScaleWorldToPixel(const raylib::Vector2& val) const;

		raylib::Camera2D mCamera;
		int32_t mPixelScale = 0;

	};
}