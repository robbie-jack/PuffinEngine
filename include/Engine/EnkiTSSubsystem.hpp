#pragma once

#include "Engine/Subsystem.hpp"

#include "TaskScheduler.h"

#include <memory>
#include <thread>

namespace puffin::core
{
	class EnkiTSSubsystem : public Subsystem
	{
	public:

		~EnkiTSSubsystem() override = default;

		void setupCallbacks() override;

		void init()
		{
			mTaskScheduler = std::make_shared<enki::TaskScheduler>();

			// Set max threads to physical - 2 (so there is some left over for other system work)
			const uint32_t maxThreads = std::thread::hardware_concurrency() - 2;

			mTaskScheduler->Initialize(maxThreads);
		}

		std::shared_ptr<enki::TaskScheduler> getTaskScheduler() { return mTaskScheduler; }

	private:

		std::shared_ptr<enki::TaskScheduler> mTaskScheduler;

	};
}