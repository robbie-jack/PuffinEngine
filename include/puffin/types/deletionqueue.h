#pragma once

#include <deque>
#include <functional>

namespace puffin
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void pushFunction(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void flush()
		{
			// Reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)(); // Call Functors
			}

			deletors.clear();
		}
	};
}
