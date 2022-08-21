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

			UIWindowSettings(Core::Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputSubsystem> InInput)
				: UIWindow(InEngine, InWorld, InInput)
			{
			};

			void Draw(float dt) override;

			inline void SetCamera(Puffin::Rendering::CameraComponent* camera) { m_camera = camera; }

		private:
			Puffin::Rendering::CameraComponent* m_camera;

		};
	}
}