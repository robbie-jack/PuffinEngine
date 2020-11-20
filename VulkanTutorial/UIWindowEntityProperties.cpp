#include "UIWindowEntityProperties.h"

#include "TransformComponent.h"
#include "ReactPhysicsComponent.h"
#include "MeshComponent.h"

#include <iostream>
#include <string>

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
						ImGui::Dummy(ImVec2(0.0f, 5.0f));
						ImGui::Text(""); ImGui::SameLine(0.0f);

						// Convert Stored String into array of char's
						std::string str_name = world->GetEntityName(entity);
						std::vector<char> name(64, '\0');
						for (int i = 0; i < str_name.size(); i++)
						{
							name[i] = str_name[i];
						}
						name.push_back('\0');

						// Display InputText and Entity ID
						if (ImGui::InputText("", &name[0], name.size(), ImGuiInputTextFlags_EnterReturnsTrue))
						{
							world->SetEntityName(entity, std::string(&name[0]));
						}

						name.clear();

						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text(" List of Components Here"); 

						// List of all Entity Components
						ImVec2 listBoxSize = ImGui::GetWindowSize();
						listBoxSize.y -= 110.0f;

						ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
							| ImGuiTreeNodeFlags_OpenOnDoubleClick
							| ImGuiTreeNodeFlags_AllowItemOverlap;

						// Display Transform Component - If One Exists
						if (world->HasComponent<TransformComponent>(entity))
						{
							TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
							float position[3] = { transform.position.x, transform.position.y, transform.position.z };
							float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
							float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Transform Component", flags))
							{
								ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

								if (ImGui::Button("X"))
								{
									world->RemoveComponent<TransformComponent>(entity);
								}

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

						// Display Mesh Component - If One Exists
						if (world->HasComponent<Rendering::MeshComponent>(entity))
						{
							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Mesh Component", flags))
							{
								ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

								if (ImGui::Button("X"))
								{
									world->RemoveComponent<Rendering::MeshComponent>(entity);
								}

								Rendering::MeshComponent& mesh = world->GetComponent<Rendering::MeshComponent>(entity);

								ImGui::Text("Model Path:"); ImGui::SameLine(100.0f);
								if (ImGui::Selectable(mesh.model_path.c_str(), false))
								{
									fileDialog->Open();
									modelSelected = true;
								}

								if (fileDialog->HasSelected() && modelSelected)
								{
									mesh.model_path = fileDialog->GetSelected().string();
									modelSelected = false;
									fileDialog->ClearSelected();

									//IO::LoadMesh(mesh, mesh.model_path);
									
								}

								ImGui::Text("Texture Path:"); ImGui::SameLine(100.0f);
								if (ImGui::Selectable(mesh.texture_path.c_str(), false))
								{
									fileDialog->Open();
									textureSelected = true;
								}

								if (fileDialog->HasSelected() && textureSelected)
								{
									mesh.texture_path = fileDialog->GetSelected().string();
									textureSelected = false;
									fileDialog->ClearSelected();
								}
							}
						}

						if (world->HasComponent<Physics::ReactPhysicsComponent>(entity))
						{
							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Physics Component", flags))
							{
								ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

								if (ImGui::Button("X"))
								{
									world->RemoveComponent<Physics::ReactPhysicsComponent>(entity);
								}
							}
						}

						ImGui::ListBoxFooter();

						ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

						if (ImGui::Button("Add Component"))
						{
							ImGui::OpenPopup("Add Component");
						}

						// Display Add Component Popup
						if (ImGui::BeginPopup("Add Component"))
						{
							if (ImGui::Selectable("Transform Component"))
							{
								if (!world->HasComponent<TransformComponent>(entity))
									world->AddComponent<TransformComponent>(entity);
							}

							if (ImGui::Selectable("Mesh Component"))
							{
								if (!world->HasComponent<Rendering::MeshComponent>(entity))
									world->AddComponent<Rendering::MeshComponent>(entity);
							}

							if (ImGui::Selectable("Physics Component"))
							{
								if (!world->HasComponent<Physics::ReactPhysicsComponent>(entity))
									world->AddComponent<Physics::ReactPhysicsComponent>(entity);
							}

							ImGui::EndPopup();
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