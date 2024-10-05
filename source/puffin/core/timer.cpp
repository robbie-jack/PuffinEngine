#include "puffin/core/timer.h"

#include "glfw/glfw3.h"

namespace puffin::core
{
	double GetTime()
	{
		return glfwGetTime();
	}

	void Timer::Start()
	{
		mStartTime = GetTime();
		mActive = true;
	}

	void Timer::End()
	{
		mEndTime = GetTime();
		mActive = false;
	}

	double Timer::GetElapsedTime() const
	{
		if (mActive)
			return GetTime() - mStartTime;

		return mEndTime - mStartTime;
	}
}
