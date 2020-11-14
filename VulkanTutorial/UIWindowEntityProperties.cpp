#include "UIWindowEntityProperties.h"
#include "TransformComponent.h"

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntityProperties::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Entity Properties";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				if (!Begin(windowName))
				{
					End();
				}
				else
				{
					if (entity != 0)
					{
						ImGui::Text(" Entity: %d", entity);
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text(" List of Components Here");

						// Display Transform Component - If One Exists
						if (world->HasComponent<TransformComponent>(entity))
						{
							TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
							float position[3] = { transform.position.x, transform.position.y, transform.position.z };
							float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
							float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Transform Component"))
							{
								if (ImGui::DragFloat3("Position", position, 0.1f))
								{
									transform.position.x = position[0];
									transform.position.y = position[1];
									transform.position.z = position[2];
								}

								if (ImGui::DragFloat3("Rotation", rotation, 0.1f))
								{
									transform.rotation.x = rotation[0];
									transform.rotation.y = rotation[1];
									transform.rotation.z = rotation[2];
								}

								if (ImGui::DragFloat3("Scale", scale, 0.1f))
								{
									transform.scale.x = scale[0];
									transform.scale.y = scale[1];
									transform.scale.z = scale[2];
								}
							}
						}
					}
					else
					{
						ImGui::Text(" No Entity Selected");
					}

					
				}
			}

			return true;
		}
	}
}