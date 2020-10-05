#pragma once
#include "UIWindow.h"
#include "Camera.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSettings : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetCamera(Puffin::Rendering::Camera* camera_) 
			{ 
				camera = camera_; 
				fov = camera->GetFov();
			};

		private:
			Puffin::Rendering::Camera* camera;
			float fov;
		};
	}
}

