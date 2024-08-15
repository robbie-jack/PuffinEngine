#pragma once

#ifndef UI_WINDOW_PERFORMANCE_H
#define UI_WINDOW_PERFORMANCE_H

#include <string>

#include "ui_window.h"

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

			UIWindowPerformance(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowPerformance() override {}

			void draw(double dt) override;

		private:

			double mFps = 0.0;
			double mFpsTimer = 0.0;
			double mFrametime = 0.0;

			//Puffin::UI::ScrollingBuffer plotBuffer;

			HardwareStats hardwareStats;

			std::unordered_map<std::string, int> m_benchmark_idx;
			std::unordered_map<std::string, std::vector<double>> m_benchmark_values;

			void draw_benchmark(const std::string& name, double benchmark_time);
		};
	}
}

#endif // UI_WINDOW_PERFORMANCE_H