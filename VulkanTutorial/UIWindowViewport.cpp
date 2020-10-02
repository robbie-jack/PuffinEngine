#include "UIWindowViewport.h"
#include "imgui/imgui_impl_vulkan.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowViewport::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				windowName = "Viewport";
				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					viewportSize = ImGui::GetWindowSize();

					// Display Scene View Texture
					ImGui::Image((ImTextureID)ImGui_ImplVulkan_AddTexture(textureSampler, sceneTexture.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), viewportSize);
				}
			}

			return true;
		}
	}
}