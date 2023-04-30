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

		void Init() override
		{
			m_taskScheduler = std::make_shared<enki::TaskScheduler>();

			m_taskScheduler->Initialize();
		}

		void Update() override {}
		void Destroy() override {}

		std::shared_ptr<enki::TaskScheduler> GetTaskScheduler() { return m_taskScheduler; }

	private:

		std::shared_ptr<enki::TaskScheduler> m_taskScheduler;

	};
}