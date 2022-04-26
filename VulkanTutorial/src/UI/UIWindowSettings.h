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

			UIWindowSettings(Engine* InEngine, std::shared_ptr<ECS::World> InWorld) : UIWindow(InEngine, InWorld)
			{
			};

			bool Draw(float dt, std::shared_ptr<Input::InputManager> InputManager) override;

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