#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace puffin::core
{
	class JobSystem
	{
		static JobSystem* s_instance;

	public:

		JobSystem() = default;

		static JobSystem* Get()
		{
			if (!s_instance)
				s_instance = new JobSystem();

			return s_instance;
		}

		static void Clear()
		{
			delete s_instance;
			s_instance = nullptr;
		}

		~JobSystem() = default;

		void Start();
		void QueueJob(const std::function<void()>& job);
		bool Wait();
		void Stop();

	private:

		uint32_t m_numThreads = 0;

		bool m_shouldTerminate = false;

		std::mutex m_queueMutex;
		std::condition_variable m_mutexCondition;

		std::vector<std::thread> m_threads;
		std::queue<std::function<void()>> m_jobs;

		void Loop();

	};
}
