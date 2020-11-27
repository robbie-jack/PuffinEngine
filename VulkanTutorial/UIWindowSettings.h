#pragma once
#include "UIWindow.h"
#include "CameraComponent.h"
#include "ProjectSettings.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetCamera(Puffin::Rendering::CameraComponent* camera_) 
			{ 
				camera = camera_; 
			};

		private:
			Puffin::Rendering::CameraComponent* camera;
			ProjectSettings settings;
		};
	}
}

