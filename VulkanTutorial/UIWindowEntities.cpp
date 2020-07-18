#include "UIWindowEntities.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntities::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				windowSize.x = 400;
				windowSize.y = 600;
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
				flags = ImGuiWindowFlags_MenuBar;

				if (!Begin("Entities"))
				{
					End();
				}
				else
				{
					// Main Menu Bar - Temporary until Editor Dockspace is implemented fully
					if (ImGui::BeginMenuBar())
					{
						if (ImGui::BeginMenu("Menu"))
						{
							if (ImGui::MenuItem("Quit", "Alt+F4"))
							{
								return false;
							}

							ImGui::EndMenu();
						}

						ImGui::EndMenuBar();
					}

					// List All Entities and their ID/Name
					ImGui::ListBoxHeader("", ImVec2(400, 600));

					for (uint32_t entityID : entityIDs)
					{
						std::string entity_string = "ID: " + std::to_string(entityID);
						if (ImGui::Selectable(entity_string.c_str(), false))
						{

						}
					}

					ImGui::ListBoxFooter();

					return true;
				}
			}
		}

		void UIWindowEntities::SetEntityIDs(std::vector<uint32_t> ids)
		{
			entityIDs = ids;
		}
	}
}