#include "UI/Editor/Windows/UIWindowViewport.h"

#include "Core/Engine.h"

#include <Components\TransformComponent.h>
#include <ManipulationGizmo.h>

namespace puffin
{
	namespace ui
	{
		void UIWindowViewport::drawWithoutImage()
		{
			mWindowName = "Viewport";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				mViewportSize = ImGui::GetWindowSize();
				mViewportSize.y -= 20.0f;

				end();
			}
		}

		void UIWindowViewport::draw(const ImTextureID textureID)
		{
			mWindowName = "Viewport";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				mFlags = ImGuiWindowFlags_MenuBar;

				begin(mWindowName);

				if (ImGui::BeginMenuBar())
				{
					static int mode = static_cast<int>(gizmoMode);

					// Manipulation Gizmo Mode
					ImGui::RadioButton("Translation", &mode, 0);
					ImGui::RadioButton("Rotation", &mode, 1);
					ImGui::RadioButton("Scale", &mode, 2);

					gizmoMode = static_cast<GizmoMode>(mode);

					// Play/Pause/Stop Buttons
					ImGui::Dummy(ImVec2((ImGui::GetWindowWidth() / 2) - 350.0f, 0.0f));
					if (ImGui::Button(mPlayButtonLabel.c_str()))
					{
						mEngine->play();

						if (const core::PlayState playState = mEngine->playState(); playState == core::PlayState::Paused || playState == core::PlayState::Stopped)
						{
							mPlayButtonLabel = "Play";
						}
						else
						{
							mPlayButtonLabel = "Pause";
						}
					}

					if (ImGui::Button("Stop"))
					{
						mEngine->restart();
						mPlayButtonLabel = "Play";
					}

					ImGui::Dummy(ImVec2(100.0f, 0.0f));

					// Compile Button
					if (ImGui::Button("Compile"))
					{
						
					}

					ImGui::EndMenuBar();
				}
				
				mViewportSize = ImGui::GetWindowSize();
				mViewportSize.y -= 40.0f;

				// Display Scene View Texture
				ImGui::Image(textureID, mViewportSize);

				if (mSelectedEntity != gInvalidID)
				{
					//TransformComponent& transform = ecsWorld->GetComponent<TransformComponent>(entity);
					
					//DrawManipulationGizmo(m_engine->getSubsystem<core::EventSubsystem>(), transform);
				}

				end();
			}
		}
	}
}