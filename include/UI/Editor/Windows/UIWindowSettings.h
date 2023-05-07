#pragma once

#include "UIWindow.h"
#include <Components/Rendering/CameraComponent.h>
#include <ProjectSettings.h>

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