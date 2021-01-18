#include "UIWindowViewport.h"
#include "imguizmo/ImGuizmo.h"

static const float identityMatrix[16] =
{ 1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f };

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

				ImGuizmo::DrawGrid((float*)&camera.matrices.view, (float*)&camera.matrices.perspective, identityMatrix, 100.0f);

				End();
			}

			return true;
		}
	}
}