#include "puffin/rendering/rendersubsystem.h"

#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"

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
