#pragma once

#include <Camera2D.hpp>

#include "puffin/rendering/rendersubsystem.h"

namespace puffin
{
	namespace utility
	{
		class Benchmark;
	}
}

namespace puffin::rendering
{
	class RenderSubsystemRL2D : public RenderSubsystem
	{
	public:

		explicit RenderSubsystemRL2D(const std::shared_ptr<core::Engine>& engine);
		~RenderSubsystemRL2D() override = default;

		void RegisterTypes() override;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		double WaitForLastPresentationAndSampleTime() override;

		void Render(double deltaTime) override;

		void WindowResized(Size size) override;
		void ViewportResized(Size size) override;

	protected:

		void DrawSprites();
		void DebugDrawStats(double deltaTime) const;
		void DebugDrawBenchmark(const utility::Benchmark* benchmark, int posX, int& posY) const;

		raylib::Camera2D mCamera;

	};
}