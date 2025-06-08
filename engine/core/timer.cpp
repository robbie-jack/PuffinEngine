#include "core/timer.h"
#include "platform.h"

namespace puffin::core
{
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
