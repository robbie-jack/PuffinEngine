#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include <UI/UIWindow.h>
#include <ECS/ECS.h>
#include <Engine.h>

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

			bool DrawWithoutImage();
			bool Draw(ImTextureID textureID, Rendering::CameraComponent& camera);

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetWorld(ECS::World* world_) { world = world_; };
			inline void SetEntity(ECS::Entity entity_) { entity = entity_; };
			inline void SetEngine(Engine* engine_) { engine = engine_; };

		private:

			ECS::Entity entity;
			ECS::World* world;
			Engine* engine;

			ImVec2 viewportSize;

			std::string playButtonLabel;
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H