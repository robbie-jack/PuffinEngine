#include "core/enkits_subsystem.h"

#include "core/engine.h"
#include "core/settings_manager.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine)
		: EngineSubsystem(engine)
	{
	}

	void EnkiTSSubsystem::PreInitialize(core::SubsystemManager* subsystemManager)
	{
		EngineSubsystem::PreInitialize(subsystemManager);

		auto* settingsManager = subsystemManager->CreateAndPreInitializeSubsystem<SettingsManager>();
	}

	void EnkiTSSubsystem::Initialize()
	{
		auto* settingsManager = m_engine->GetSubsystem<SettingsManager>();

		mThreadCount = settingsManager->Get<uint32_t>("general", "thread_count").value_or(4);
		
		mTaskScheduler = std::make_shared<enki::TaskScheduler>();
		mTaskScheduler->Initialize(mThreadCount);
	}

	void EnkiTSSubsystem::Deinitialize()
	{
		mTaskScheduler->WaitforAllAndShutdown();
		mTaskScheduler = nullptr;
	}

	std::string_view EnkiTSSubsystem::GetName() const
	{
		return reflection::GetTypeString<EnkiTSSubsystem>();
	}

	std::shared_ptr<enki::TaskScheduler> EnkiTSSubsystem::GetTaskScheduler()
	{
		return mTaskScheduler;
	}

	uint32_t EnkiTSSubsystem::GetThreadCount() const
	{
		return mThreadCount;
	}
}
