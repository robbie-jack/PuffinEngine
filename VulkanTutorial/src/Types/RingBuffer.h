#pragma once

namespace Puffin
{
	// Ring Buffer Interface

	// Ring buffer to hold events
	template<typename T>
	class RingBuffer
	{
	public:

		RingBuffer()
		{
			MAX_PENDING = 16;
			queue.resize(MAX_PENDING);

			head = 0;
			tail = head;
		}

		// Push new event onto back of queue
		void Push(T event)
		{
			if ((tail + 1) % MAX_PENDING == head)
			{
				Resize();
			}

			// Add event to end of list
			queue[tail] = event;
			tail = (tail + 1) % MAX_PENDING;
		}

		// Pop event off front of queue
		bool Pop(T& event)
		{
			// Return false if there are no events in queue
			if (head == tail) return false;

			event = queue[head];

			head = (head + 1) % MAX_PENDING;

			return true;
		}

		// Flushes all stored objects
		void Flush()
		{
			head = 0;
			tail = head;
		}

		// Check if buffer is empty
		bool IsEmpty()
		{
			return head == tail;
		}

	private:
		int MAX_PENDING;
		int head;
		int tail;

		std::vector<T> queue;

		// Resizes queue
		void Resize()
		{
			// Copy elements to another vector
			std::vector<T> oldQueue = queue;

			// Double size of queue
			MAX_PENDING *= 2;
			queue.resize(MAX_PENDING);

			// Iterate over each event in old queue
			for (int i = 0; i < oldQueue.size(); i++)
			{
				// Get index into old queue starting from head
				int index = (head + i) % oldQueue.size();

				// For each event, add to front of resized queue
				queue[i] = oldQueue[index];
			}

			// After copying events back into queue, set head to 0 and tail to the size of old queue so it as after last element
			head = 0;
			tail = oldQueue.size();
		}
	};
}