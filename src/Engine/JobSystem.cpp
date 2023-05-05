#include "Engine/JobSystem.hpp"

namespace puffin::core
{
	JobSystem* JobSystem::s_instance = nullptr;

	void JobSystem::Start()
	{
		m_numThreads = std::thread::hardware_concurrency() - 1;

		m_threads.resize(m_numThreads);

		for (uint32_t i = 0; i < m_numThreads; i++)
		{
			m_threads.emplace_back(&JobSystem::Loop, this);
		}
	}

	void JobSystem::QueueJob(const std::function<void()>& job)
	{
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_jobs.push(job);
		}

		m_mutexCondition.notify_one();
	}

	bool JobSystem::Wait()
	{
		bool poolBusy;

		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			poolBusy = m_jobs.empty();
		}

		return poolBusy;
	}

	void JobSystem::Stop()
	{
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_shouldTerminate = true;
		}

		m_mutexCondition.notify_all();

		for (auto& activeThread : m_threads)
		{
			activeThread.join();
		}

		m_threads.clear();
	}

	void JobSystem::Loop()
	{
		while (true)
		{
			std::function<void()> job;

			{
				std::unique_lock<std::mutex> lock(m_queueMutex);

				m_mutexCondition.wait(lock, [this]
					{
						return !m_jobs.empty() || m_shouldTerminate;
					});

				if (m_shouldTerminate)
					return;

				job = m_jobs.front();
				m_jobs.pop();
			}

			job();
		}
	}
}