#include "ui/windows/ui_window_viewport.h"

#include "imgui.h"
#include "puffin/core/engine.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/manipulationgizmo.h"

namespace puffin
{
	namespace ui
	{
		UIWindowViewport::UIWindowViewport(std::shared_ptr<core::Engine> engine): UIWindow(engine)
		{
		}

		void UIWindowViewport::DrawWithoutImage()
		{
			mWindowName = "Viewport";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				Begin(mWindowName);

				mViewportSize = ImGui::GetWindowSize();
				mViewportSize.y -= 20.0f;

				End();
			}
		}

		void UIWindowViewport::Draw(const ImTextureID textureID)
		{
			mWindowName = "Viewport";

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				mFlags = ImGuiWindowFlags_MenuBar;

				Begin(mWindowName);

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
						m_engine->Play();
					}

					const core::PlayState playState = m_engine->GetPlayState();
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
						m_engine->Restart();
						mPlayButtonLabel = "Play";
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

				End();
			}
		}

		void UIWindowViewport::Draw(double deltaTime)
		{
		}

		ImVec2 UIWindowViewport::GetViewportSize() const
		{
			return mViewportSize;
		}
	}
}