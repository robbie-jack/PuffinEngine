#pragma once

#ifndef UI_WINDOW_PERFORMANCE_H
#define UI_WINDOW_PERFORMANCE_H

#include <string>

#include "uiwindow.h"

namespace puffin
{
	namespace ui
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
			int logicalCores{}, physicalCores{}, vramTotal{}, ramTotal{};
			float vramUsage{}, ramUsage{};
		};

		class UIWindowPerformance : public UIWindow
		{
		public:

			UIWindowPerformance(std::shared_ptr<core::Engine> engine);
			~UIWindowPerformance() override = default;

			void Draw(double deltaTime) override;

		private:

			double mFps = 0.0;
			double mFpsTimer = 0.0;
			double mFrameTime = 0.0;

			//Puffin::UI::ScrollingBuffer plotBuffer;

			HardwareStats hardwareStats;

			std::unordered_map<std::string, int> mBenchmarkIdx;
			std::unordered_map<std::string, std::vector<double>> mBenchmarkValues;

			void DrawBenchmark(const std::string& name);
		};
	}
}

#endif // UI_WINDOW_PERFORMANCE_H