#include "UIWindowEntityProperties.h"

#include "TransformComponent.h"
#include "MeshComponent.h"
#include "RigidbodyComponent.h"

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
					if (entity != ECS::INVALID_ENTITY)
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
							float PI = 3.14159;

							TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
							float position[3] = { transform.position.x, transform.position.y, transform.position.z };
							float rotation[3] = { 
								transform.rotation.x * 180 / PI, 
								transform.rotation.y * 180 / PI, 
								transform.rotation.z * 180 / PI };
							float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Transform Component", flags))
							{
								ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

								ImGui::Button("X");

								if (ImGui::IsItemActivated())
								{
									world->RemoveComponent<TransformComponent>(entity);
								}

								if (ImGui::DragFloat3("Position", position, 0.1f))
								{
									transform.position.x = position[0];
									transform.position.y = position[1];
									transform.position.z = position[2];

									positionChanged = true;
								}

								if (ImGui::DragFloat3("Rotation", rotation, 0.1f))
								{
									transform.rotation.x = rotation[0] * PI / 180;
									transform.rotation.y = rotation[1] * PI / 180;
									transform.rotation.z = rotation[2] * PI / 180;
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

								Rendering::MeshComponent& mesh = world->GetComponent<Rendering::MeshComponent>(entity);

								ImGui::Button("X");

								if (ImGui::IsItemClicked())
								{
									mesh.flag_deleted = true;
								}

								// Change Model Path
								ImGui::Text("Model Path:"); ImGui::SameLine(100.0f);
								if (ImGui::Selectable(mesh.model_path.c_str(), false))
								{
									fileDialog->Open();
									modelSelected = true;
								}

								if (fileDialog->HasSelected() && modelSelected)
								{
									mesh.model_path = fileDialog->GetSelected().string();
									mesh.flag_created = true;
									modelSelected = false;
									fileDialog->ClearSelected();
								}

								// Change Texture Path
								ImGui::Text("Texture Path:"); ImGui::SameLine(100.0f);
								if (ImGui::Selectable(mesh.texture_path.c_str(), false))
								{
									fileDialog->Open();
									textureSelected = true;
								}

								if (fileDialog->HasSelected() && textureSelected)
								{
									mesh.texture_path = fileDialog->GetSelected().string();
									mesh.flag_created = true;
									textureSelected = false;
									fileDialog->ClearSelected();
								}
							}
						}

						if (world->HasComponent<Physics::RigidbodyComponent>(entity))
						{
							ImGui::SetNextItemOpen(true, ImGuiCond_Once);
							if (ImGui::CollapsingHeader("Physics Component", flags))
							{
								ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

								Physics::RigidbodyComponent& comp = world->GetComponent<Physics::RigidbodyComponent>(entity);

								ImGui::Button("X");

								if (ImGui::IsItemClicked())
								{
									comp.flag_deleted = true;
								}

								if (positionChanged)
								{
									comp.flag_created = true;
								}
							}
						}

						positionChanged = false;

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
								{
									world->AddComponent<TransformComponent>(entity);
								}
							}

							if (ImGui::Selectable("Mesh Component"))
							{
								if (!world->HasComponent<Rendering::MeshComponent>(entity))
								{
									Rendering::MeshComponent& comp = world->AddComponent<Rendering::MeshComponent>(entity);
									comp.model_path = "models\\cube.obj";
									comp.texture_path = "textures\\cube.png";
									comp.flag_created = true;
								}
							}

							if (ImGui::Selectable("Physics Component"))
							{
								if (!world->HasComponent<Physics::RigidbodyComponent>(entity))
								{
									Physics::RigidbodyComponent& comp = world->AddComponent<Physics::RigidbodyComponent>(entity);
									comp.size = btVector3(1.0f, 1.0f, 1.0f);
									comp.mass = 0.0f;
									comp.flag_created = true;
								}
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