#include "puffin/core/enkitssubsystem.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine) : Subsystem(engine)
	{
		mName = "EnkiTSSubsystem";
	}

	void EnkiTSSubsystem::Initialize(SubsystemManager* subsystemManager)
	{
		mTaskScheduler = std::make_shared<enki::TaskScheduler>();

		// Set max threads to physical - 2 (so there is some left over for other system work)
		const uint32_t maxThreads = std::thread::hardware_concurrency() - 2;

		mTaskScheduler->Initialize(maxThreads);
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
}
