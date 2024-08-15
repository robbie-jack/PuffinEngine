#include "puffin/core/enkitssubsystem.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine) : Subsystem(engine)
	{
		mName = "EnkiTSSubsystem";
	}

	EnkiTSSubsystem::~EnkiTSSubsystem()
	{
		mEngine = nullptr;
	}

	void EnkiTSSubsystem::Initialize(SubsystemManager* subsystem_manager)
	{
		m_task_scheduler = std::make_shared<enki::TaskScheduler>();

		// Set max threads to physical - 2 (so there is some left over for other system work)
		const uint32_t max_threads = std::thread::hardware_concurrency() - 2;

		m_task_scheduler->Initialize(max_threads);
	}

	std::shared_ptr<enki::TaskScheduler> EnkiTSSubsystem::get_task_scheduler()
	{
		return m_task_scheduler;
	}
}
