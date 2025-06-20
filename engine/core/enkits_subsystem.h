#pragma once

#include <memory>
#include <thread>

#include "core/subsystem.h"

#include "TaskScheduler.h"

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
		uint32_t GetThreadCount() const;

	private:

		std::shared_ptr<enki::TaskScheduler> mTaskScheduler;

		uint32_t mThreadCount = 0;

	};
}