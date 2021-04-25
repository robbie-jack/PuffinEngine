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

			UIWindowViewport(Engine* InEngine, std::shared_ptr<ECS::World> InWorld) : UIWindow(InEngine, InWorld)
			{
			};

			bool DrawWithoutImage();
			bool Draw(ImTextureID textureID, Rendering::CameraComponent& camera);

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetEntity(ECS::Entity entity_) { entity = entity_; };

		private:

			ECS::Entity entity;

			ImVec2 viewportSize;

			std::string playButtonLabel = "Play";
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H