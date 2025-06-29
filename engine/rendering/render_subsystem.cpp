#include "rendering/render_subsystem.h"

#include "core/settings_manager.h"
#include "core/signal_subsystem.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(const std::shared_ptr<core::Engine>& engine)
		: Subsystem(engine)
	{
	}

	void RenderSubsystem::Initialize()
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

	void RenderSubsystem::InitSettingsAndSignals()
	{
		auto* settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
		auto* signalSubsystem = mEngine->GetSubsystem<core::SignalSubsystem>();

		// Physics Interpolation
		{
			mRenderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();

			auto enablePhysicsInterpolationSignal = signalSubsystem->GetOrCreateSignal("rendering_physics_interpolation_enable");
			enablePhysicsInterpolationSignal->Connect(std::function([&]
			{
				auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();

				mRenderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();
			}));
		}
	}
}
