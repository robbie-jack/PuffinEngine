#include "JobManager.h"

#include <utility>

namespace Puffin
{
	namespace Job
	{
		void JobManager::Init()
		{
			// Initialize Available IDs
			for (uint32_t i = 0; i < MAX_JOBS; i++)
			{
				availableIDs.push(i);
			}

			// Create Worker for each available thread - 1
			// this leaves the main thread free
			//int numThreads = std::thread::hardware_concurrency() - 1;
			int numThreads = 1; // Only one thread while testing JobManager

			for (int i = 0; i < numThreads; i++)
			{
				std::thread worker([&]
				{
					Job job; // Current job for this thread

					// Infinite Loop for Thread
					while (true)
					{
						// If job found, execute it
						if (GetJob(job))
						{
							job.func();

							jobMutex.lock();

							// Update Job status
							jobStatus[job.id] = JobStatus::FINISHED;

							jobMutex.unlock();

							finishedLabel.fetch_add(1); // Update worker label state
						}
						// No job, put thread to sleep
						else
						{
							std::unique_lock<std::mutex> lock(wakeMutex);
							wakeCondition.wait(lock);
						}
					}
				});

				// Let thread process in background
				worker.detach();
			}
		}

		// Add job to end of queue
		uint32_t JobManager::QueueJob(const std::function<void(void)>& func)
		{
			// Create Job
			Job job;
			job.func = func;
			job.id = availableIDs.front();

			availableIDs.pop();

			// Update main thread state
			currentLabel++;

			while (!AddJob(job))
			{
				Poll();
			}

			wakeCondition.notify_one(); // Wake one thread

			return job.id;
		}

		void JobManager::WaitForJob(uint32_t id)
		{
			bool jobFinished = false;
			while (!jobFinished)
			{
				if (GetJobStatus(id) == JobStatus::FINISHED)
				{
					jobFinished = true;
				}
			}

			return;
		}
		
		JobStatus JobManager::GetJobStatus(uint32_t id)
		{
			// Return current status of job if it is valid
			if (jobStatus.find(id) != jobStatus.end())
			{
				return jobStatus[id];
			}
			// if not valid return Invalid flag
			else
			{
				return JobStatus::INVALID;
			}
		}

		void JobManager::GetJobResult()
		{

		}

		void JobManager::Cleanup()
		{
			
		}

		bool JobManager::AddJob(const Job& job)
		{
			bool result = false;
			jobMutex.lock();

			// Add Job to Queue
			jobQueue.push(job);

			// Update Job Status
			jobStatus.insert({ job.id, JobStatus::QUEUED });

			jobMutex.unlock();
			return result;
		}

		bool JobManager::GetJob(Job& job)
		{
			bool result = false;
			jobMutex.lock(); // Lock Queue

			// If there are pending jobs in queue
			if (!jobQueue.empty())
			{
				// Pass job back to thread
				job = std::move(jobQueue.front());
				jobQueue.pop();

				// Update Job status
				jobStatus[job.id] = JobStatus::PROCESSING;

				result = true;
			}

			jobMutex.unlock(); // Unlock Queue
			return result;
		}

		void JobManager::Poll()
		{
			wakeCondition.notify_one(); // Wake one worker thread
			std::this_thread::yield(); // Allow thread to be rescheduled
		}

		bool JobManager::IsBusy()
		{
			return finishedLabel.load() < currentLabel;
		}

		void JobManager::Wait()
		{
			while (IsBusy())
			{
				Poll();
			}
		}
	}
}