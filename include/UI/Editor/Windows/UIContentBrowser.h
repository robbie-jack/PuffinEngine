#pragma once

#include "UIWindow.h"

namespace puffin::ui
{
	class UIContentBrowser : public UIWindow
	{
	public:
			
		UIContentBrowser(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine)
		{
			mWindowName = "Content Browser";
		}
		~UIContentBrowser() override {}

		void draw(double dt) override;
	};
}