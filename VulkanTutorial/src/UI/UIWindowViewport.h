#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include "UIWindow.h"
#include "../Rendering/VKTypes.h"
#include "../ECS/ECS.h"
#include "../Rendering/Components/CameraComponent.h"

//#include <vulkan/vulkan.h>

namespace Puffin
{
	namespace UI
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			bool DrawWithoutImage();
			bool Draw(ImTextureID textureID, Rendering::CameraComponent& camera);

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetWorld(ECS::World* world_) { world = world_; };
			inline void SetEntity(ECS::Entity entity_) { entity = entity_; };

		private:

			ECS::Entity entity;
			ECS::World* world;

			ImVec2 viewportSize;
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H