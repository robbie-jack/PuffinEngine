#pragma once

#include <vector>

namespace puffin
{
	// Ring Buffer Interface

	// Ring buffer to hold events
	template<typename T>
	class RingBuffer
	{
	public:

		RingBuffer()
		{
			mMaxPending = 16;
			mQueue.resize(mMaxPending);

			mHead = 0;
			mTail = mHead;
		}

		// Push new event onto back of queue
		void push(T event)
		{
			if ((mTail + 1) % mMaxPending == mHead)
			{
				resize();
			}

			// Add event to end of list
			mQueue[mTail] = event;
			mTail = (mTail + 1) % mMaxPending;
		}

		// Pop event off front of queue
		bool pop(T& event)
		{
			// Return false if there are no events in queue
			if (mHead == mTail) return false;

			event = mQueue[mHead];

			mHead = (mHead + 1) % mMaxPending;

			return true;
		}

		// Flushes all stored objects
		void flush()
		{
			mHead = 0;
			mTail = mHead;
		}

		// Check if buffer is empty
		bool isEmpty() const
		{
			return mHead == mTail;
		}

	private:
		int mMaxPending;
		int mHead;
		int mTail;

		std::vector<T> mQueue;

		// Resizes queue
		void resize()
		{
			// Copy elements to another vector
			std::vector<T> oldQueue = mQueue;

			// Double size of queue
			mMaxPending *= 2;
			mQueue.resize(mMaxPending);

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