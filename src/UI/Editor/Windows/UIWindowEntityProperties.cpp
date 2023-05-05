#include "UI/Editor/Windows/UIWindowEntityProperties.h"

#include "Engine\Engine.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components\Scripting\AngelScriptComponent.h"
#include "Components\Procedural\ProceduralMeshComponent.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"

#include "ECS/ECS.h"

#include "Types/ComponentFlags.h"

#include <misc/cpp/imgui_stdlib.h>

#include <iostream>
#include <string>

namespace puffin
{
	namespace UI
	{
		void UIWindowEntityProperties::Draw(double dt)
		{
			windowName = "Entity Properties";

			auto ecsWorld = m_engine->getSubsystem<ECS::World>();

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				if (m_entity != ECS::INVALID_ENTITY)
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					// Edit Entity Name
					std::string name = ecsWorld->GetEntityName(m_entity);
					std::string* namePtr = &name;
					if (ImGui::InputText("##Name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						ecsWorld->SetEntityName(m_entity, *namePtr);
					}

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" List of Components Here");

					// List of all Entity Components
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 110.0f;

					if (ImGui::BeginListBox("##EntityProperties", listBoxSize))
					{
						ImGuiTreeNodeFlags flags = 0;
						//flags |= ImGuiTreeNodeFlags_Framed;
						//flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
						flags |= ImGuiTreeNodeFlags_NoAutoOpenOnLog;
						flags |= ImGuiTreeNodeFlags_OpenOnArrow;
						flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
						flags |= ImGuiTreeNodeFlags_AllowItemOverlap;
						flags |= ImGuiTreeNodeFlags_DefaultOpen;
						flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

						sceneChanged = false;

						// Display Component UI
						DrawTransformUI(flags);

						DrawMeshUI(flags);

						DrawLightUI(flags);
						DrawShadowcasterUI(flags);

						DrawProceduralPlaneUI(flags);

						DrawRigidbody2DUI(flags);
						DrawCircle2DUI(flags);
						DrawBox2DUI(flags);

						DrawScriptUI(flags);

						positionChanged = false;

						ImGui::EndListBox();
					}

					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

					if (ImGui::Button("Add Component"))
					{
						ImGui::OpenPopup("Add Component");
					}

					// Display Add Component Popup
					if (ImGui::BeginPopup("Add Component"))
					{
						if (ImGui::Selectable("Transform"))
						{
							if (!ecsWorld->HasComponent<TransformComponent>(m_entity))
							{
								ecsWorld->AddComponent<TransformComponent>(m_entity);
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Static Mesh"))
						{
							if (!ecsWorld->HasComponent<rendering::MeshComponent>(m_entity))
							{
								ecsWorld->AddComponent<rendering::MeshComponent>(m_entity);
								sceneChanged = true;
							}
						}

						ImGui::Separator();

						if (ImGui::Selectable("Light"))
						{
							ecsWorld->AddComponent<rendering::LightComponent>(m_entity);
						}

						if (ImGui::Selectable("Shadow Caster"))
						{
							ecsWorld->AddComponent<rendering::ShadowCasterComponent>(m_entity);
						}

						ImGui::Separator();
							
						if (ImGui::Selectable("Rigidbody"))
						{
							ecsWorld->AddComponent<physics::RigidbodyComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Circle2D"))
						{
							ecsWorld->AddComponent<physics::CircleComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Box2D"))
						{
							ecsWorld->AddComponent<physics::BoxComponent2D>(m_entity);
						}

						ImGui::EndPopup();
					}
				}
				else
				{
					ImGui::Text(" No Entity Selected");
				}

				End();
			}
		}

		void UIWindowEntityProperties::DrawTransformUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();

			// Display Transform Component - If One Exists
			if (ecsWorld->HasComponent<TransformComponent>(m_entity))
			{
				TransformComponent& transform = ecsWorld->GetComponent<TransformComponent>(m_entity);

				if (ImGui::TreeNodeEx("Transform Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Transform"))
					{
						ecsWorld->RemoveComponent<TransformComponent>(m_entity);
						sceneChanged = true;
					}

					#ifdef PFN_USE_DOUBLE_PRECISION
						if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &transform.position, 3, 0.1f))
						{
							positionChanged = true;
							sceneChanged = true;
						}
					#else
						if (ImGui::DragFloat3("Position", (float*)&transform.position, 0.1f))
						{
							positionChanged = true;
							sceneChanged = true;
						}
					#endif

					if (ImGui::DragFloat3("Rotation", (float*)&transform.rotation, 0.1f))
					{
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Scale", (float*)&transform.scale, 0.1f))
					{
						sceneChanged = true;
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawMeshUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();

			// Display Mesh Component - If One Exists
			if (ecsWorld->HasComponent<rendering::MeshComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Mesh Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					rendering::MeshComponent& mesh = ecsWorld->GetComponent<rendering::MeshComponent>(m_entity);

					if (ImGui::SmallButton("X##Mesh"))
					{
						ecsWorld->SetComponentFlag<rendering::MeshComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					ImGui::Text("Model UUID: %d", mesh.meshAssetId);
					ImGui::Text("Texture UUID: %d", mesh.textureAssetId);

					// Change Model Path
					/*ImGui::Text("Model Path:"); ImGui::SameLine();
					if (ImGui::Selectable((const char*)mesh.model_path.c_str(), false))
					{
						fileDialog->Open();
						modelSelected = true;
					}

					if (fileDialog->HasSelected() && modelSelected)
					{
						mesh.model_path = fileDialog->GetSelected().string();
						world->SetComponentInitialized<Rendering::MeshComponent>(entity, false);

						modelSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}*/

					// Change Texture Path
					/*ImGui::Text("Texture Path:"); ImGui::SameLine();
					if (ImGui::Selectable((const char*)mesh.texture_path.c_str(), false))
					{
						fileDialog->Open();
						textureSelected = true;
					}

					if (fileDialog->HasSelected() && textureSelected)
					{
						mesh.texture_path = fileDialog->GetSelected().string();
						world->SetComponentInitialized<Rendering::MeshComponent>(entity, false);

						textureSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}*/

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawLightUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<rendering::LightComponent>(m_entity))
			{
				auto& light = ecsWorld->GetComponent<rendering::LightComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Point Light"))
					{
						ecsWorld->SetComponentFlag<rendering::LightComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					int item_current_idx = static_cast<int>(light.type);

					// Edit Light Type
					const char* comboLabel = rendering::gLightTypeLabels[item_current_idx].c_str();
					if (ImGui::BeginCombo("Type", comboLabel))
					{
						for (int i = 0; i < rendering::gLightTypeLabels.size(); i++)
						{
							const bool is_selected = (item_current_idx == i);

							if (ImGui::Selectable(rendering::gLightTypeLabels[i].c_str(), is_selected))
							{
								item_current_idx = i;

								light.type = static_cast<rendering::LightType>(item_current_idx);

								dirty = true;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					// Edit Light Diffuse Color
					dirty |= ImGui::ColorEdit3("Diffuse", (float*)&light.color);

					// Edit Light Ambient Intensity
					dirty |= ImGui::DragFloat("Ambient Intensity", &light.ambientIntensity);

					// Edit Light Specular Intensity
					dirty |= ImGui::DragFloat("Specular Intensity", &light.specularIntensity);

					// Edit Light Specular Exponent
					dirty |= ImGui::DragInt("Specular Exponent", &light.specularExponent);

					if (light.type == rendering::LightType::Point || light.type == rendering::LightType::Spot)
					{
						dirty |= ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, .01f, .01f, 1.f, "%.4f");

						dirty |= ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, .01f, .01f, 2.f, "%.6f");
					}

					if (light.type == rendering::LightType::Spot)
					{
						dirty |= ImGui::DragFloat("Inner Cutoff Angle", &light.innerCutoffAngle, 0.25f, 0.0f, 45.0f);

						// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
						dirty |= ImGui::DragFloat("Outer Cutoff Angle", &light.outerCutoffAngle, 0.25f, light.innerCutoffAngle, 45.0f);

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (light.outerCutoffAngle < light.innerCutoffAngle)
						{
							light.outerCutoffAngle = light.innerCutoffAngle;
						}
					}

					ImGui::TreePop();
				}

				if (dirty)
				{
					sceneChanged = true;
					ecsWorld->SetComponentFlag<rendering::LightComponent, FlagDirty>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawShadowcasterUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<rendering::ShadowCasterComponent>(m_entity))
			{
				auto& shadowcaster = ecsWorld->GetComponent<rendering::ShadowCasterComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Shadow Caster"))
					{
						ecsWorld->SetComponentFlag<rendering::ShadowCasterComponent, FlagDeleted>(m_entity, true);
						sceneChanged = true;
					}

					int item_current_idx = 0;
					for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
					{
						if (rendering::gShadowResolutionValues[i] == shadowcaster.shadowmapWidth)
						{
							item_current_idx = i;
							break;
						}
					}

					const char* label = rendering::gShadowResolutionLabels[item_current_idx].c_str();
					if (ImGui::BeginCombo("Shadow Resolution", label))
					{
						for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
						{
							const bool is_selected = (item_current_idx == i);

							if (ImGui::Selectable(rendering::gShadowResolutionLabels[i].c_str(), is_selected))
							{
								item_current_idx = i;

								shadowcaster.shadowmapWidth = rendering::gShadowResolutionValues[item_current_idx];
								shadowcaster.shadowmapHeight = rendering::gShadowResolutionValues[item_current_idx];

								dirty = true;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					ImGui::TreePop();
				}

				if (dirty)
				{
					sceneChanged = true;
					ecsWorld->SetComponentFlag<rendering::ShadowCasterComponent, FlagDirty>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawProceduralPlaneUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<procedural::PlaneComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Procedural Plane Component"), flags)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& plane = ecsWorld->GetComponent<procedural::PlaneComponent>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##ProceduralPlane"))
					{
						ecsWorld->SetComponentFlag<procedural::PlaneComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat2("Half Size", (float*)&plane.halfSize, 0.1f);
					dirty |= ImGui::DragInt2("Num Quads", (int*)&plane.numQuads);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<procedural::PlaneComponent, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawRigidbody2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<physics::RigidbodyComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Rigidbody Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& rb = ecsWorld->GetComponent<physics::RigidbodyComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Rigidbody"))
					{
						ecsWorld->SetComponentFlag<physics::RigidbodyComponent2D, FlagDeleted>(m_entity, true);
							
						sceneChanged = true;
					}

					// Combo box for Body Type Selection
					const char* items[] = { "Static", "Kinematic", "Dynamic" };
					int item_current_idx = static_cast<int>(rb.bodyType);
					const char* label = items[item_current_idx];
					if (ImGui::BeginCombo("Body Type", label))
					{
						for (int i = 0; i < IM_ARRAYSIZE(items); i++)
						{
							const bool isSelected = (item_current_idx == i);
							if (ImGui::Selectable(items[i], isSelected))
							{
								item_current_idx = i;
								rb.bodyType = static_cast<physics::BodyType>(item_current_idx);
								dirty = true;
							}

							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}

						ImGui::EndCombo();
					}

					dirty |= ImGui::DragFloat("Mass", &rb.mass);
					dirty |= ImGui::DragFloat("Elasticity", &rb.elasticity);
					//dirty |= ImGui::Checkbox("Sleeping Allowed", &sleep);
					//dirty |= ImGui::Checkbox("Bullet", &bullet);
					//dirty |= ImGui::Checkbox("Awake", &awake);
					//dirty |= ImGui::Checkbox("Rotation Fixed", &fixedRotation);
					//dirty |= ImGui::DragFloat("Angular Damping", &angularDamping);
					//dirty |= ImGui::DragFloat("Linear Damping", &linearDamping);
					//dirty |= ImGui::DragFloat("Gravity Scale", &gravityScale);

					if(dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawCircle2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<physics::CircleComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Circle Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& circle = ecsWorld->GetComponent<physics::CircleComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Circle2D"))
					{
						ecsWorld->SetComponentFlag<physics::CircleComponent2D, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat("Radius", &circle.radius);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawBox2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<physics::BoxComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Box Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& box = ecsWorld->GetComponent<physics::BoxComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Box2D"))
					{
						ecsWorld->SetComponentFlag< physics::BoxComponent2D, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&box.halfExtent), 0.1f, 0.0f);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawScriptUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->getSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<scripting::AngelScriptComponent>(m_entity))
			{
				scripting::AngelScriptComponent& comp = ecsWorld->GetComponent<scripting::AngelScriptComponent>(m_entity);

				if (ImGui::TreeNodeEx("Script Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Script"))
					{
						ecsWorld->SetComponentFlag<scripting::AngelScriptComponent, FlagDeleted>(m_entity, true);
						sceneChanged = true;
					}

					ImGui::InputText("Script Name", &comp.name);

					ImGui::Text("File Path:"); ImGui::SameLine(0.0f);
					if (ImGui::Selectable(comp.dir.string().c_str(), false))
					{
						//fileDialog->Open();
						//modelSelected = true;
					}

					if (comp.obj != 0)
					{
						ImGui::Separator();
						ImGui::Text("Editable Properties");

						// Display Variables in scripts marked as editable
						for (const int& index : comp.editableProperties)
						{
							// Get Property Name/Type
							const char* propertyName = comp.obj->GetPropertyName(index);
							int propertyType = comp.obj->GetPropertyTypeId(index);

							// Display property with correct UI based on its type
							if (propertyType == asTYPEID_BOOL)
							{
								bool* boolProperty = (bool*)comp.obj->GetAddressOfProperty(index);

								ImGui::Checkbox(propertyName, boolProperty);
							}
							else if (propertyType == asTYPEID_INT32)
							{
								int* intProperty = (int*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputInt(propertyName, intProperty);
							}
							else if (propertyType == asTYPEID_FLOAT)
							{
								float* floatProperty = (float*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputFloat(propertyName, floatProperty);
							}
							else if (propertyType == asTYPEID_DOUBLE)
							{
								double* doubleProperty = (double*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputDouble(propertyName, doubleProperty);
							}
							else
							{
								std::string* stringProperty = (std::string*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputText(propertyName, stringProperty);
							}
						}

						ImGui::Separator();
						ImGui::Text("Visible Properties");

						// Display Variables in scripts marked as visible
						for (const int& index : comp.visibleProperties)
						{
							// Get Property Name/Type
							const char* propertyName = comp.obj->GetPropertyName(index);
							int propertyType = comp.obj->GetPropertyTypeId(index);

							std::string propertyText = propertyName;

							// Display property with correct UI based on its type
							if (propertyType == asTYPEID_INT32)
							{
								int intProperty = *(int*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %d";

								ImGui::Text(propertyText.c_str(), intProperty);
							}
							else if (propertyType == asTYPEID_FLOAT)
							{
								float floatProperty = *(float*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %.20f";

								ImGui::Text(propertyText.c_str(), floatProperty);
							}
							else if (propertyType == asTYPEID_DOUBLE)
							{
								double doubleProperty = *(double*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %.20f";

								ImGui::Text(propertyText.c_str(), doubleProperty);
							}
							else
							{
								std::string stringProperty = *(std::string*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %s";

								ImGui::Text(propertyText.c_str(), stringProperty.c_str());
							}
						}
					}

					ImGui::TreePop();
				}
			}
		}
	}
}