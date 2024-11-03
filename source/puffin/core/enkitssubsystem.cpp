#include "puffin/core/enkitssubsystem.h"

#include "puffin/core/engine.h"
#include "puffin/core/settingsmanager.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine) : Subsystem(engine)
	{
		mName = "EnkiTSSubsystem";
	}

	void EnkiTSSubsystem::Initialize(SubsystemManager* subsystemManager)
	{
		auto settingsManager = subsystemManager->CreateAndInitializeSubsystem<SettingsManager>();

		mThreadCount = settingsManager->Get<uint32_t>("general", "thread_count").value_or(4);
		
		mTaskScheduler = std::make_shared<enki::TaskScheduler>();
		mTaskScheduler->Initialize(mThreadCount);
	}

	void EnkiTSSubsystem::Deinitialize()
	{
		mTaskScheduler->WaitforAllAndShutdown();
		mTaskScheduler = nullptr;
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
