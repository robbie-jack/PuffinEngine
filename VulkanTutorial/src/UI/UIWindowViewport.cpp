#include <UI/UIWindowViewport.h>

namespace Puffin
{
	namespace UI
	{
		bool UIWindowViewport::DrawWithoutImage()
		{
			windowName = "Viewport";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				viewportSize = ImGui::GetWindowSize();
				viewportSize.y -= 20.0f;

				End();
			}

			return true;
		}

		bool UIWindowViewport::Draw(ImTextureID textureID, Rendering::CameraComponent& camera)
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

				if (entity != ECS::INVALID_ENTITY)
				{

				}



				End();
			}

			return true;
		}
	}
}