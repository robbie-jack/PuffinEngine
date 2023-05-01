#pragma once

#include "Engine/Subsystem.hpp"

#include "TaskScheduler.h"

#include <memory>
#include <thread>

namespace Puffin::Core
{
	class EnkiTSSubsystem : public Subsystem
	{
	public:

		~EnkiTSSubsystem() override = default;

		void SetupCallbacks() override;

		void Init()
		{
			m_taskScheduler = std::make_shared<enki::TaskScheduler>();

			// Set max threads to physical - 2 (so there is some left over for other system work)
			const uint32_t maxThreads = std::thread::hardware_concurrency() - 2;

			m_taskScheduler->Initialize(maxThreads);
		}

		std::shared_ptr<enki::TaskScheduler> GetTaskScheduler() { return m_taskScheduler; }

	private:

		std::shared_ptr<enki::TaskScheduler> m_taskScheduler;

	};
}