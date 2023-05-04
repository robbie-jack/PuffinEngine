#pragma once

#include "UIWindow.h"
#include <Components/Rendering/CameraComponent.h>
#include <ProjectSettings.h>

namespace puffin
{
	class Engine;

	namespace UI
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			UIWindowSettings(std::shared_ptr<Core::Engine> engine) : UIWindow(engine) {}
			~UIWindowSettings() override {}

			void Draw(double dt) override;

			inline void SetCamera(puffin::Rendering::CameraComponent* camera) { m_camera = camera; }

		private:

			puffin::Rendering::CameraComponent* m_camera = nullptr;

		};
	}
}