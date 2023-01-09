#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include "UIWindow.h"

#include <string>

namespace Puffin
{
	namespace UI
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			UIWindowViewport(std::shared_ptr<Core::Engine> engine) : UIWindow(engine) {}
			~UIWindowViewport() override {}

			void DrawWithoutImage();
			void Draw(ImTextureID textureID);
			void Draw(double dt) override {}

			inline ImVec2 GetViewportSize() { return viewportSize; }

			inline void SetEntity(ECS::EntityID entity_) { entity = entity_; }

		private:

			ECS::EntityID entity = ECS::INVALID_ENTITY;

			ImVec2 viewportSize = ImVec2(640.0f, 480.0f);

			std::string playButtonLabel = "Play";
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H