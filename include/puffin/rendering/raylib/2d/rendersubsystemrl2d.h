#pragma once

#include "puffin/rendering/rendersubsystem.h"

//#include "raylibcpp/raylib-cpp.hpp"

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

	protected:



	};
}