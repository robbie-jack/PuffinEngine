#include "puffin/core/enkits_subsystem.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	EnkiTSSubsystem::EnkiTSSubsystem(const std::shared_ptr<Engine>& engine) : EngineSubsystem(engine)
	{
		
	}

	EnkiTSSubsystem::~EnkiTSSubsystem()
	{

	}

	void EnkiTSSubsystem::initialize(core::ISubsystemManager* subsystem_manager)
	{
		EngineSubsystem::initialize(subsystem_manager);

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
