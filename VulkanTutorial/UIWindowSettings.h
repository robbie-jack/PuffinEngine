#pragma once
#include "UIWindow.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;
		};
	}
}

