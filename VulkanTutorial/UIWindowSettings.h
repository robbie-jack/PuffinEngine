#pragma once
#include "UIWindow.h"
#include "CameraComponent.h"

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
				fov = camera->fov;
			};

		private:
			Puffin::Rendering::CameraComponent* camera;
			float fov;
		};
	}
}

