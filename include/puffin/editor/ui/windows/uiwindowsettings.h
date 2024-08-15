#pragma once

#include "puffin/editor/ui/windows/uiwindow.h"
#include "puffin/components/rendering/cameracomponent.h"

namespace puffin
{
	class Engine;

	namespace ui
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			UIWindowSettings(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowSettings() override {}

			void draw(double dt) override;

		private:

			puffin::rendering::CameraComponent3D* mCamera = nullptr;

		};
	}
}
