#include "UIWindowSceneView.h"
#include "imgui/imgui_impl_vulkan.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowSceneView::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				windowSize.x = 400;
				windowSize.y = 600;
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

				if (!Begin("Scene View"))
				{
					End();
				}
				else
				{
					// Test Text
					ImGui::Text("Test");

					// Display Scene View Texture
					//ImGui::Image((ImTextureID)ImGui_ImplVulkan_AddTexture(textureSampler, sceneTexture.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), windowSize);

					return true;
				}

				return true;
			}
		}
	}
}