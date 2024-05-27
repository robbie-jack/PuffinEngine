#pragma once

#include "puffin/core/system.h"

#include "TaskScheduler.h"

#include <memory>
#include <thread>

namespace puffin::core
{
	class EnkiTSSubsystem : public System
	{
	public:

		EnkiTSSubsystem(const std::shared_ptr<Engine>& engine);
		~EnkiTSSubsystem() override { m_engine = nullptr; }

		void startup();

		std::shared_ptr<enki::TaskScheduler> getTaskScheduler() { return mTaskScheduler; }

	private:

		std::shared_ptr<enki::TaskScheduler> mTaskScheduler;

	};
}