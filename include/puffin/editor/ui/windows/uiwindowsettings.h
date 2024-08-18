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

			explicit UIWindowSettings(std::shared_ptr<core::Engine> engine);
			~UIWindowSettings() override = default;

			void Draw(double deltaTime) override;

		private:

			puffin::rendering::CameraComponent3D* mCamera = nullptr;

		};
	}
}
