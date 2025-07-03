#include "rendering/render_subsystem.h"

#include "core/settings_manager.h"
#include "core/signal_subsystem.h"

namespace puffin::rendering
{
	RenderSubsystem::RenderSubsystem(const std::shared_ptr<core::Engine>& engine)
		: EngineSubsystem(engine)
	{
	}

	void RenderSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		EngineSubsystem::Initialize(subsystemManager);

		subsystemManager->CreateAndInitializeSubsystem<core::SettingsManager>();
		subsystemManager->CreateAndInitializeSubsystem<core::SignalSubsystem>();

		InitSettingsAndSignals();
	}

	std::string_view RenderSubsystem::GetName() const
	{
		return EngineSubsystem::GetName();
	}

	double RenderSubsystem::WaitForLastPresentationAndSampleTime()
	{
		return 0.0;
	}

	void RenderSubsystem::Render(double deltaTime)
	{
	}

	void RenderSubsystem::InitSettingsAndSignals()
	{
		auto* settingsManager = m_engine->GetSubsystem<core::SettingsManager>();
		auto* signalSubsystem = m_engine->GetSubsystem<core::SignalSubsystem>();

		// Physics Interpolation
		{
			mRenderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();

			auto enablePhysicsInterpolationSignal = signalSubsystem->GetOrCreateSignal("rendering_physics_interpolation_enable");
			enablePhysicsInterpolationSignal->Connect(std::function([&]
			{
				auto settingsManager = m_engine->GetSubsystem<core::SettingsManager>();

				mRenderSettings.physicsInterpolationEnable = settingsManager->Get<bool>("rendering", "physics_interpolation_enable").value();
			}));
		}
	}
}
