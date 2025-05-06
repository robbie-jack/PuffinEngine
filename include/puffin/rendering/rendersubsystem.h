#pragma once

#include <unordered_map>

#include "puffin/core/subsystem.h"
#include "puffin/types/size.h"

namespace puffin::rendering
{
	struct RenderSettings
	{
		Size renderResolution = { 0, 0 };
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

		const Size& GetResolution() const;
		void SetResolution(const Size& resolution);

	protected:

		virtual void UpdateResolution(const Size& resolution) = 0;

		RenderSettings renderSettings;

		uint32_t mFrameCount = 0;

		uint32_t mDrawCallsCountTotal = 0; // Total count of all draw calls
		std::unordered_map<std::string, uint32_t> mDrawCallsCount; // Count for individual draw call type

	private:

		void InitSettingsAndSignals();

	};
}
