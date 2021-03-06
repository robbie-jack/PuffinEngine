#include <UI/UIWindowViewport.h>
#include <Components\TransformComponent.h>
#include <ManipulationGizmo.h>
#include <ECS/ECS.h>
#include <Engine.h>

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

				flags = ImGuiWindowFlags_MenuBar;

				Begin(windowName);

				if (ImGui::BeginMenuBar())
				{
					static int mode = (int)gizmoMode;

					// Manipulation Gizmo Mode
					ImGui::RadioButton("Translation", &mode, 0);
					ImGui::RadioButton("Rotation", &mode, 1);
					ImGui::RadioButton("Scale", &mode, 2);

					gizmoMode = (GizmoMode)mode;

					// Play/Pause/Stop Buttons
					ImGui::Dummy(ImVec2((ImGui::GetWindowWidth() / 2) - 350.0f, 0.0f));
					if (ImGui::Button(playButtonLabel.c_str()))
					{
						engine->Play();

						switch (engine->GetPlayState())
						{
						case PlayState::PAUSED:
							playButtonLabel = "Play";
							break;
						case PlayState::PLAYING:
							playButtonLabel = "Pause";
							break;
						}
					}

					if (ImGui::Button("Stop"))
					{
						engine->Restart();
						playButtonLabel = "Play";
					}

					ImGui::EndMenuBar();
				}
				
				viewportSize = ImGui::GetWindowSize();
				viewportSize.y -= 40.0f;

				// Display Scene View Texture
				ImGui::Image(textureID, viewportSize);

				if (entity != ECS::INVALID_ENTITY)
				{
					TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
					
					DrawManipulationGizmo(world, transform);
				}

				End();
			}

			return true;
		}
	}
}