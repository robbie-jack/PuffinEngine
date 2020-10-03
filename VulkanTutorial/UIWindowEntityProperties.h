#pragma once
#include "UIWindow.h"

namespace Puffin
{
    namespace UI
    {
        class UIWindowEntityProperties : public UIWindow
        {
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;
        };
    }
}

