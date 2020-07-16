#pragma once

#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowMenu : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

		private:

			float fps;
			float fps_timer;
		};
	}
}