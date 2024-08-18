#pragma once

#include "puffin/editor/ui/windows/uiwindow.h"

namespace puffin
{
	namespace rendering
	{
		struct CameraComponent3D;
	}

	class Engine;

	namespace ui
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			UIWindowSettings(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowSettings() override {}

			void Draw(double dt) override;

		private:

			puffin::rendering::CameraComponent3D* mCamera = nullptr;

		};
	}
}
