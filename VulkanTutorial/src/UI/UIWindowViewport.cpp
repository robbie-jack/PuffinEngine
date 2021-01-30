#include <UI/UIWindowViewport.h>
#include <Rendering\DebugDraw.h>
#include <Components\TransformComponent.h>

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
					TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
					float lineLength = 2.5f;

					Debug::DrawLine(transform.position, transform.position + Vector3(0.0f, 0.0f, lineLength), Vector3(1.0f, 0.0f, 0.0f));
					Debug::DrawLine(transform.position, transform.position + Vector3(0.0f, lineLength, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
					Debug::DrawLine(transform.position, transform.position + Vector3(lineLength, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
				}

				End();
			}

			return true;
		}
	}
}