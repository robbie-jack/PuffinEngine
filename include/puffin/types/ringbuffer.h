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
			m_max_pending = 16;
			m_queue.resize(m_max_pending);

			m_head = 0;
			m_tail = m_head;
		}

		// Push new event onto back of queue
		void push(T event)
		{
			if ((m_tail + 1) % m_max_pending == m_head)
			{
				resize();
			}

			// Add event to end of list
			m_queue[m_tail] = event;
			m_tail = (m_tail + 1) % m_max_pending;
		}

		// Pop event off front of queue
		bool pop(T& event)
		{
			// Return false if there are no events in queue
			if (m_head == m_tail) return false;

			event = m_queue[m_head];

			m_head = (m_head + 1) % m_max_pending;

			return true;
		}

		// Flushes all stored objects
		void flush()
		{
			m_head = 0;
			m_tail = m_head;
		}

		// Check if buffer is empty
		bool empty() const
		{
			return m_head == m_tail;
		}

	private:
		int m_max_pending;
		int m_head;
		int m_tail;

		std::vector<T> m_queue;

		// Resizes queue
		void resize()
		{
			// Copy elements to another vector
			std::vector<T> oldQueue = m_queue;

			// Double size of queue
			m_max_pending *= 2;
			m_queue.resize(m_max_pending);

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