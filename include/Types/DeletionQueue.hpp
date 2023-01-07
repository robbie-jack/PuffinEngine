#pragma once

#include <deque>
#include <functional>

namespace Puffin
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void PushFunction(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void Flush()
		{
			// Reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)(); // Call Functors
			}

			deletors.clear();
		}
	};
}
