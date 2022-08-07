#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include "UIWindow.h"
#include <Types\Vector.h>
#include <Rendering/VKTypes.h>
#include <Components/Rendering/CameraComponent.h>

#include <string>

namespace Puffin
{
	namespace UI
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			UIWindowViewport(Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput)
			: UIWindow(InEngine, InWorld, InInput)
			{
			};

			void DrawWithoutImage();
			void Draw(ImTextureID textureID);
			void Draw(float dt) override {}

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetEntity(ECS::EntityID entity_) { entity = entity_; }

		private:

			ECS::EntityID entity;

			ImVec2 viewportSize;

			std::string playButtonLabel = "Play";
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H