#include "puffin/core/enkits_subsystem.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine) : System(engine)
	{
		mEngine->registerCallback(core::ExecutionStage::Startup, [&]() { startup(); }, "EnkiTSSubsystem: Startup", 50);
	}

	void EnkiTSSubsystem::startup()
	{
		mTaskScheduler = std::make_shared<enki::TaskScheduler>();

		// Set max threads to physical - 2 (so there is some left over for other system work)
		const uint32_t maxThreads = std::thread::hardware_concurrency() - 2;

		mTaskScheduler->Initialize(maxThreads);
	}
}
