#include "puffin/rendering/rendersubsystem.h"

#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/platform/raylib/window/windowsubsystemrl.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(const std::shared_ptr<core::Engine>& engine)
		: Subsystem(engine)
	{
	}

	void RenderSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		Subsystem::Initialize(subsystemManager);

		InitSettingsAndSignals();
	}

	double RenderSubsystem::WaitForLastPresentationAndSampleTime()
	{
		return 0.0;
	}

	void RenderSubsystem::Render(double deltaTime)
	{

	}

	core::SubsystemType RenderSubsystem::GetType() const
	{
		return core::SubsystemType::Render;
	}

	const Size& RenderSubsystem::GetResolution() const
	{
		return renderSettings.renderResolution;
	}

	void RenderSubsystem::SetResolution(const Size& resolution)
	{
		renderSettings.renderResolution = resolution;

		UpdateResolution(resolution);

		auto* settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		settingsManager->Set("rendering", "render_width", renderSettings.renderResolution.width);
		settingsManager->Set("rendering", "render_height", renderSettings.renderResolution.height);
	}

	void RenderSubsystem::InitSettingsAndSignals()
	{
		auto* settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto* signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();
		auto* windowSubsystem = mEngine->GetSubsystem<window::WindowSubsystemRL>();

		// Render Resolution
		{
			renderSettings.renderResolution.width = settingsManager->Get<int>("rendering", "render_width").value();
			renderSettings.renderResolution.height = settingsManager->Get<int>("rendering", "render_height").value();

			if (renderSettings.renderResolution.width == 0)
			{
				renderSettings.renderResolution.width = windowSubsystem->GetPrimaryWindowWidth();
			}

			if (renderSettings.renderResolution.height == 0)
			{
				renderSettings.renderResolution.height = windowSubsystem->GetPrimaryWindowHeight();
			}

			auto setResFunc = std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				Size resolution;
				resolution.width = settingsManager->Get<int>("rendering", "render_width").value();
				resolution.height = settingsManager->Get<int>("rendering", "render_height").value();

				if (renderSettings.renderResolution != resolution)
					SetResolution(resolution);
				
			});

			signalSubsystem->GetOrCreateSignal("rendering_render_width")->Connect(setResFunc);
			signalSubsystem->GetOrCreateSignal("rendering_render_height")->Connect(setResFunc);
		}

		// Physics Interpolation
		{
			renderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();

			auto enablePhysicsInterpolationSignal = signalSubsystem->GetOrCreateSignal("rendering_physics_interpolation_enable");
			enablePhysicsInterpolationSignal->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				renderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();
			}));
		}
	}
}
