#pragma once

#ifndef UI_WINDOW_SETTINGS_H
#define UI_WINDOW_SETTINGS_H

#include <UI/UIWindow.h>
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

			UIWindowSettings(Engine* InEngine, ECS::World* InWorld);

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetCamera(Puffin::Rendering::CameraComponent* camera_) 
			{ 
				camera = camera_; 
			};

		private:
			Puffin::Rendering::CameraComponent* camera;

		};
	}
}

#endif // UI_WINDOW_SETTINGS_H