#pragma once
#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
		// utility structure for realtime plot
		struct ScrollingBuffer {
			int MaxSize;
			int Offset;
			ImVector<ImVec2> Data;
			ScrollingBuffer() {
				MaxSize = 2000;
				Offset = 0;
				Data.reserve(MaxSize);
			}
			void AddPoint(float x, float y) {
				if (Data.size() < MaxSize)
					Data.push_back(ImVec2(x, y));
				else {
					Data[Offset] = ImVec2(x, y);
					Offset = (Offset + 1) % MaxSize;
				}
			}
			void Erase() {
				if (Data.size() > 0) {
					Data.shrink(0);
					Offset = 0;
				}
			}
		};

		class UIWindowPerformance : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

		private:
			int fps;
			float fps_timer;
			Puffin::UI::ScrollingBuffer plotBuffer;
		};
	}
}

