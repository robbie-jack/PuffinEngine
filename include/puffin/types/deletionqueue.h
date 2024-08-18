#pragma once

#include <deque>
#include <functional>

namespace puffin
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void PushFunction(std::function<void()>&& function);
		void Flush();
	};
}
