#pragma once

namespace puffin::core
{
	class Timer
	{
	public:

		void Start();
		void End();
		[[nodiscard]] double GetElapsedTime() const;

	private:

		double mStartTime = 0.0;
		double mEndTime = 0.0;

		bool mActive = false;

	};
}