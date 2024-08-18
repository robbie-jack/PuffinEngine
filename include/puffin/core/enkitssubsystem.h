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
		~EnkiTSSubsystem() override = default;

		void Initialize(SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		std::shared_ptr<enki::TaskScheduler> GetTaskScheduler();

	private:

		std::shared_ptr<enki::TaskScheduler> mTaskScheduler;

	};
}