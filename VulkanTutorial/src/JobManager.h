#pragma once

#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include <map>
#include <queue>
#include <vector>
#include <thread>
#include <algorithm>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Puffin
{
	namespace Job
	{
		struct Job
		{
			uint32_t id; // ID for this job
			std::function<void(void)> func; // Function to perform for this job
		};

		enum class JobStatus
		{
			INVALID = 0, // This job is invalid
			QUEUED = 1, // Job is currently queued and waiting on a free thread
			PROCESSING = 2, // Job is being processed by a thread
			FINISHED = 3 // Job is finished
		};

		class Worker
		{
			std::thread thread;
			Job activeJob; // Job this worker is currently performing
		};

		const uint32_t MAX_JOBS = 10000;

		class JobManager
		{
		public:

			void Init();

			uint32_t QueueJob(const std::function<void(void)>& func);
			void WaitForJob(uint32_t id);
			JobStatus GetJobStatus(uint32_t id);
			void GetJobResult();

			void Cleanup();

		private:

			bool AddJob(const Job& job);
			bool GetJob(Job& job);
			void Poll();
			bool IsBusy();
			void Wait();

			std::queue<uint32_t> availableIDs; // Queue of jobs ids that are available
			std::queue<Job> jobQueue; // Queue of jobs to be completed
			//std::vector<Worker> workers; // Vector of workers, equal to number of logical threads - 1
			std::map<uint32_t, JobStatus> jobStatus;

			std::mutex jobMutex; // Used to lock job queue when a thread is accessing it
			std::mutex wakeMutex; // Used with Condition Variable to sleep jobs when there is nothing to do
			std::condition_variable wakeCondition; // Used with mutex

			uint64_t currentLabel = 0; // Track execution state of main thread
			std::atomic<uint64_t> finishedLabel; // Track execution state of background worker threads
		};
	}
}

#endif // JOB_MANAGER_H