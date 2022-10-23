#pragma once

#include <vector>

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
			m_maxPending = 16;
			m_queue.resize(m_maxPending);

			m_head = 0;
			m_tail = m_head;
		}

		// Push new event onto back of queue
		void Push(T event)
		{
			if ((m_tail + 1) % m_maxPending == m_head)
			{
				Resize();
			}

			// Add event to end of list
			m_queue[m_tail] = event;
			m_tail = (m_tail + 1) % m_maxPending;
		}

		// Pop event off front of queue
		bool Pop(T& event)
		{
			// Return false if there are no events in queue
			if (m_head == m_tail) return false;

			event = m_queue[m_head];

			m_head = (m_head + 1) % m_maxPending;

			return true;
		}

		// Flushes all stored objects
		void Flush()
		{
			m_head = 0;
			m_tail = m_head;
		}

		// Check if buffer is empty
		bool IsEmpty()
		{
			return m_head == m_tail;
		}

	private:
		int m_maxPending;
		int m_head;
		int m_tail;

		std::vector<T> m_queue;

		// Resizes queue
		void Resize()
		{
			// Copy elements to another vector
			std::vector<T> oldQueue = m_queue;

			// Double size of queue
			m_maxPending *= 2;
			m_queue.resize(m_maxPending);

			// Iterate over each event in old queue
			for (int i = 0; i < oldQueue.size(); i++)
			{
				// Get index into old queue starting from head
				int index = (m_head + i) % oldQueue.size();

				// For each event, add to front of resized queue
				m_queue[i] = oldQueue[index];
			}

			// After copying events back into queue, set head to 0 and tail to the size of old queue so it as after last element
			m_head = 0;
			m_tail = oldQueue.size();
		}
	};
}