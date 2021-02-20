#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include <UI/UIWindow.h>
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

			UIWindowViewport(Engine* InEngine, ECS::World* InWorld);

			bool DrawWithoutImage();
			bool Draw(ImTextureID textureID, Rendering::CameraComponent& camera);

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetEntity(ECS::Entity entity_) { entity = entity_; };

		private:

			ECS::Entity entity;

			ImVec2 viewportSize;

			std::string playButtonLabel;
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H