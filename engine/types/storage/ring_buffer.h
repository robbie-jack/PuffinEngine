#pragma once

#include <vector>
#include <mutex>

namespace puffin
{
	// Ring Buffer Interface

	// Ring buffer to hold events
	template<typename T>
	class RingBuffer
	{
	public:

		RingBuffer(uint64_t defaultSize = 32)
		{
			mSize = defaultSize;
			mQueue.resize(mSize);

			mHead = 0;
			mTail = mHead;
		}

		// Push new event onto back of queue
		void Push(const T& event)
		{
			mLock.lock();
			
			if ((mTail + 1) % mSize == mHead)
			{
				Resize();
			}

			// Add event to end of list
			mQueue[mTail] = event;
			mTail = (mTail + 1) % mSize;

			mLock.unlock();
		}

		// Pop event off front of queue
		bool Pop(T& event)
		{
			// Return false if there are no events in queue
			if (mHead == mTail) return false;

			mLock.lock();
			
			event = mQueue[mHead];

			mHead = (mHead + 1) % mSize;

			mLock.unlock();

			return true;
		}

		// Flushes all stored objects
		void Flush()
		{
			mLock.lock();
			
			mHead = 0;
			mTail = mHead;
			
			mLock.unlock();
		}

		// Check if buffer is empty
		bool Empty() const
		{
			return mHead == mTail;
		}

	private:

		uint64_t mSize;
		uint64_t mHead;
		uint64_t mTail;
		std::mutex mLock;

		std::vector<T> mQueue;

		// Resizes queue
		void Resize()
		{
			// Copy elements to another vector
			std::vector<T> oldQueue = mQueue;

			// Double size of queue
			mSize *= 2;
			mQueue.resize(mSize);

			// Iterate over each event in old queue
			for (int i = 0; i < oldQueue.size(); i++)
			{
				// Get index into old queue starting from head
				int index = (mHead + i) % oldQueue.size();

				// For each event, add to front of resized queue
				mQueue[i] = oldQueue[index];
			}

			// After copying events back into queue, set head to 0 and tail to the size of old queue so it as after last element
			mHead = 0;
			mTail = oldQueue.size();
		}
	};
}