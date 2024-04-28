#include "puffin/ui/editor/windows/ui_window_viewport.h"

#include "imgui.h"
#include "puffin/core/engine.h"
#include "puffin/components/transform_component_2d.h"
#include "puffin/manipulation_gizmo.h"

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
					}

					const core::PlayState playState = mEngine->playState();
					if (playState == core::PlayState::Playing)
					{
						mPlayButtonLabel = "Pause";
					}
					else
					{
						mPlayButtonLabel = "Play";
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

				mViewportSize = ImGui::GetContentRegionAvail();

				// Display Scene View Texture
				ImGui::Image(textureID, mViewportSize);

				if (mSelectedEntity != gInvalidID)
				{
					//TransformComponent3D& transform = ecsWorld->GetComponent<TransformComponent3D>(entity);
					
					//DrawManipulationGizmo(m_engine->getSystem<core::EventSubsystem>(), transform);
				}

				end();
			}
		}
	}
}