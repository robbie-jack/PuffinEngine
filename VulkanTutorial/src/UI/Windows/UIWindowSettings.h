#pragma once

#include "UIWindow.h"
#include <Components/Rendering/CameraComponent.h>
#include <ProjectSettings.h>

namespace Puffin
{
	class Engine;

	namespace UI
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			UIWindowSettings(std::shared_ptr<Core::Engine> engine) : UIWindow(engine) {}
			~UIWindowSettings() override {}

			void Draw(float dt) override;

			inline void SetCamera(Puffin::Rendering::CameraComponent* camera) { m_camera = camera; }

		private:

			Puffin::Rendering::CameraComponent* m_camera = nullptr;

		};
	}
}