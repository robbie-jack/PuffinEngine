#pragma once

#ifndef UI_WINDOW_PERFORMANCE_H
#define UI_WINDOW_PERFORMANCE_H

#include "UIWindow.h"

namespace puffin
{
	namespace UI
	{
		//// utility structure for realtime plot
		//struct ScrollingBuffer {
		//	int MaxSize;
		//	int Offset;
		//	ImVector<ImVec2> Data;
		//	ScrollingBuffer() {
		//		MaxSize = 2000;
		//		Offset = 0;
		//		Data.reserve(MaxSize);
		//	}
		//	void AddPoint(float x, float y) {
		//		if (Data.size() < MaxSize)
		//			Data.push_back(ImVec2(x, y));
		//		else {
		//			Data[Offset] = ImVec2(x, y);
		//			Offset = (Offset + 1) % MaxSize;
		//		}
		//	}
		//	void Erase() {
		//		if (Data.size() > 0) {
		//			Data.shrink(0);
		//			Offset = 0;
		//		}
		//	}
		//};

		struct HardwareStats
		{
			std::string cpuName, gpuName;
			int logicalCores, physicalCores, vramTotal, ramTotal;
			float vramUsage, ramUsage;
		};

		class UIWindowPerformance : public UIWindow
		{
		public:

			UIWindowPerformance(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowPerformance() override {}

			void Draw(double dt) override;

		private:
			double fps = 0.0f;
			double fps_timer = 0.0f;
			double frametime = 0.0f;
			//Puffin::UI::ScrollingBuffer plotBuffer;

			HardwareStats hardwareStats;
		};
	}
}

#endif // UI_WINDOW_PERFORMANCE_H