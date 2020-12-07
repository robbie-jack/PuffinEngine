#include "UIWindowViewport.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowViewport::Draw(ImTextureID textureID)
		{
			windowName = "Viewport";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);
				
				viewportSize = ImGui::GetWindowSize();
				viewportSize.y -= 20.0f;

				// Display Scene View Texture
				ImGui::Image(textureID, viewportSize);

				End();
			}

			return true;
		}
	}
}