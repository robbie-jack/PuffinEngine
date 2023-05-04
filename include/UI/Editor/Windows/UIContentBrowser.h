#pragma once

#include "UIWindow.h"

namespace puffin::UI
{
	class UIContentBrowser : public UIWindow
	{
	public:
			
		UIContentBrowser(std::shared_ptr<Core::Engine> engine) : UIWindow(engine)
		{
			windowName = "Content Browser";
		}
		~UIContentBrowser() override {}

		void Draw(double dt) override;
	};
}