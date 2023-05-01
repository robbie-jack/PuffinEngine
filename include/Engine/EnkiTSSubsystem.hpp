#pragma once

#include "Engine/Subsystem.hpp"

#include "TaskScheduler.h"

#include <memory>

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

			m_taskScheduler->Initialize();
		}

		std::shared_ptr<enki::TaskScheduler> GetTaskScheduler() { return m_taskScheduler; }

	private:

		std::shared_ptr<enki::TaskScheduler> m_taskScheduler;

	};
}