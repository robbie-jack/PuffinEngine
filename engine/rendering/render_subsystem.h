﻿#pragma once

#include <unordered_map>

#include "core/subsystem.h"
#include "types/size.h"
#include "types/vector3.h"

namespace puffin::rendering
{
	struct RenderSettings
	{
		bool physicsInterpolationEnable = false;
	};

	/*
	 * A Render Subsystem implements a rendering api (i.e Vulkan, DirectX, etc...) and
	 * handles common rendering functionality like resource management for that api
	 * Explicit rendering functionality like for a forward render pass, is handled in the renderer classes
	 */
	class RenderSubsystem : public core::Subsystem
	{
	public:

		explicit RenderSubsystem(const std::shared_ptr<core::Engine>& engine);
		~RenderSubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;

		/*
		 * Called each frame to wait for last presentation to complete and sample frame time
		 */
		virtual double WaitForLastPresentationAndSampleTime();

		/*
		 * Called each frame to render 2d/3d scene to display
		 */
		virtual void Render(double deltaTime);

		[[nodiscard]] core::SubsystemType GetType() const override;

		virtual void WindowResized(Size size) = 0;
		virtual void ViewportResized(Size size) = 0;

		virtual void DrawTextToScreen(const std::string& string, int posX, int posY, int fontSize = 20, Vector3f color = { 1.0f }) = 0;

	protected:

		RenderSettings mRenderSettings;

		uint32_t mFrameCount = 0;

		uint32_t mDrawCallsCountTotal = 0; // Total count of all draw calls
		std::unordered_map<std::string, uint32_t> mDrawCallsCount; // Count for individual draw call type

	private:

		void InitSettingsAndSignals();

	};
}
