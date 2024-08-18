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

			explicit UIWindowViewport(std::shared_ptr<core::Engine> engine);
			~UIWindowViewport() override = default;

			void DrawWithoutImage();
			void Draw(ImTextureID textureID);
			void Draw(double deltaTime) override;

			[[nodiscard]] ImVec2 GetViewportSize() const;

			//inline void SetEntity(ECS::EntityID entity_) { entity = entity_; }

		private:

			ImVec2 mViewportSize = ImVec2(640.0f, 480.0f);

			std::string mPlayButtonLabel = "Play";
		};
	}
}
