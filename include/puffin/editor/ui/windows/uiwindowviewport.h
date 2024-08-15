#pragma once

#include <string>

#include "puffin/editor/ui/windows/uiwindow.h"
#include "puffin/types/uuid.h"

namespace puffin
{
	namespace ui
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			UIWindowViewport(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowViewport() override {}

			void drawWithoutImage();
			void draw(ImTextureID textureID);
			void draw(double dt) override {}

			[[nodiscard]] ImVec2 viewportSize() const { return mViewportSize; }

			//inline void SetEntity(ECS::EntityID entity_) { entity = entity_; }

		private:

			ImVec2 mViewportSize = ImVec2(640.0f, 480.0f);

			std::string mPlayButtonLabel = "Play";
		};
	}
}
