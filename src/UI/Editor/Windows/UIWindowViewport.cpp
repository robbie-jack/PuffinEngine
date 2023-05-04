#include "UI/Editor/Windows/UIWindowViewport.h"

#include "Engine/Engine.hpp"

#include <Components\TransformComponent.h>
#include <ManipulationGizmo.h>
#include <ECS/ECS.h>

namespace puffin
{
	namespace UI
	{
		void UIWindowViewport::DrawWithoutImage()
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
		}

		void UIWindowViewport::Draw(ImTextureID textureID)
		{
			windowName = "Viewport";

			auto ecsWorld = m_engine->getSubsystem<ECS::World>();

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
						m_engine->play();

						Core::PlayState playState = m_engine->playState();
						if (playState == Core::PlayState::paused || playState == Core::PlayState::stopped)
						{
							playButtonLabel = "Play";
						}
						else
						{
							playButtonLabel = "Pause";
						}
					}

					if (ImGui::Button("Stop"))
					{
						m_engine->restart();
						playButtonLabel = "Play";
					}

					ImGui::Dummy(ImVec2(100.0f, 0.0f));

					// Compile Button
					if (ImGui::Button("Compile"))
					{
						
					}

					ImGui::EndMenuBar();
				}
				
				viewportSize = ImGui::GetWindowSize();
				viewportSize.y -= 40.0f;

				// Display Scene View Texture
				ImGui::Image(textureID, viewportSize);

				if (entity != ECS::INVALID_ENTITY)
				{
					TransformComponent& transform = ecsWorld->GetComponent<TransformComponent>(entity);
					
					DrawManipulationGizmo(m_engine->getSubsystem<Core::EventSubsystem>(), transform);
				}

				End();
			}
		}
	}
}