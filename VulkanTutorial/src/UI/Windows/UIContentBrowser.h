#pragma once

#include "UIWindow.h"

namespace Puffin::UI
{
	class UIContentBrowser : public UIWindow
	{
	public:
			
		UIContentBrowser(Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput)
			: UIWindow(InEngine, InWorld, InInput)
		{
			windowName = "Content Browser";
		}
		~UIContentBrowser() {}

		void Draw(float dt) override;
	};
}