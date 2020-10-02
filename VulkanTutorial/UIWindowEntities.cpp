#include "UIWindowEntities.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntities::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

				windowName = "Entities";
				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					// List All Entities and their ID/Name
					ImGui::ListBoxHeader("", ImGui::GetWindowSize()); // Make ListBox fill Window

					for (uint32_t entityID : entityIDs)
					{
						std::string entity_string = "ID: " + std::to_string(entityID);
						if (ImGui::Selectable(entity_string.c_str(), false))
						{

						}
					}

					ImGui::ListBoxFooter();
				}
			}

			return true;
		}

		void UIWindowEntities::SetEntityIDs(std::vector<uint32_t> ids)
		{
			entityIDs = ids;
		}
	}
}