#include "puffin/types/deletionqueue.h"

namespace puffin
{
	void DeletionQueue::PushFunction(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void DeletionQueue::Flush()
	{
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); // Call Functors
		}

		deletors.clear();
	}
}
