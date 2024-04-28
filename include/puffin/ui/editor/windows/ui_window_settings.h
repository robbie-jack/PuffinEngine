#pragma once

#include "puffin/ui/editor/windows/ui_window.h"
#include "puffin/components/rendering/camera_component.h"

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

			puffin::rendering::CameraComponent* mCamera = nullptr;

		};
	}
}
