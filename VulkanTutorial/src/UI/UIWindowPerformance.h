#pragma once

#ifndef UI_WINDOW_PERFORMANCE_H
#define UI_WINDOW_PERFORMANCE_H

#include <UI/UIWindow.h>

namespace Puffin
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
			UIWindowPerformance(Engine* InEngine, std::shared_ptr<ECS::World> InWorld) : UIWindow(InEngine, InWorld)
			{
			};

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

		private:
			float fps;
			float fps_timer, frametime;
			//Puffin::UI::ScrollingBuffer plotBuffer;

			HardwareStats hardwareStats;
		};
	}
}

#endif // UI_WINDOW_PERFORMANCE_H