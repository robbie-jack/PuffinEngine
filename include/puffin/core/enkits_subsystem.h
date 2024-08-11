#pragma once

#include "puffin/core/subsystem.h"

#include "TaskScheduler.h"

#include <memory>
#include <thread>

namespace puffin::core
{
	class EnkiTSSubsystem : public Subsystem
	{
	public:

		explicit EnkiTSSubsystem(const std::shared_ptr<Engine>& engine);
		~EnkiTSSubsystem() override;

		void initialize(SubsystemManager* subsystem_manager) override;

		std::shared_ptr<enki::TaskScheduler> get_task_scheduler();

	private:

		std::shared_ptr<enki::TaskScheduler> m_task_scheduler;

	};
}