#include "UI/Editor/Windows/UIWindowEntityProperties.h"

#include "Engine/Engine.hpp"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components/Scripting/AngelScriptComponent.hpp"
#include "Components/Procedural/ProceduralMeshComponent.hpp"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"

#include "ECS/ECS.h"

#include "Types/ComponentFlags.h"

#include <misc/cpp/imgui_stdlib.h>

#include <iostream>
#include <string>

namespace Puffin
{
	namespace UI
	{
		void UIWindowEntityProperties::Draw(double dt)
		{
			windowName = "Entity Properties";

			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

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

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

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

					ImGui::ListBoxFooter();

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
							if (!ecsWorld->HasComponent<Rendering::MeshComponent>(m_entity))
							{
								ecsWorld->AddComponent<Rendering::MeshComponent>(m_entity);
								sceneChanged = true;
							}
						}

						ImGui::Separator();

						if (ImGui::Selectable("Light"))
						{
							ecsWorld->AddComponent<Rendering::LightComponent>(m_entity);
						}

						if (ImGui::Selectable("Shadow Caster"))
						{
							ecsWorld->AddComponent<Rendering::ShadowCasterComponent>(m_entity);
						}

						ImGui::Separator();
							
						if (ImGui::Selectable("Rigidbody"))
						{
							ecsWorld->AddComponent<Physics::RigidbodyComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Circle2D"))
						{
							ecsWorld->AddComponent<Physics::CircleComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Box2D"))
						{
							ecsWorld->AddComponent<Physics::BoxComponent2D>(m_entity);
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
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

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
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

			// Display Mesh Component - If One Exists
			if (ecsWorld->HasComponent<Rendering::MeshComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Mesh Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Rendering::MeshComponent& mesh = ecsWorld->GetComponent<Rendering::MeshComponent>(m_entity);

					if (ImGui::SmallButton("X##Mesh"))
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Rendering::MeshComponent, FlagDeleted>(m_entity, true);
					}

					ImGui::Text("Model UUID: %d", mesh.meshAssetID);
					ImGui::Text("Texture UUID: %d", mesh.textureAssetID);

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
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Rendering::LightComponent>(m_entity))
			{
				auto& light = ecsWorld->GetComponent<Rendering::LightComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Point Light"))
					{
						ecsWorld->SetComponentFlag<Rendering::LightComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					int item_current_idx = static_cast<int>(light.type);

					// Edit Light Type
					const char* comboLabel = Rendering::LIGHT_TYPE_LABELS[item_current_idx].c_str();
					if (ImGui::BeginCombo("Type", comboLabel))
					{
						for (int i = 0; i < Rendering::LIGHT_TYPE_LABELS.size(); i++)
						{
							const bool is_selected = (item_current_idx == i);

							if (ImGui::Selectable(Rendering::LIGHT_TYPE_LABELS[i].c_str(), is_selected))
							{
								item_current_idx = i;

								light.type = static_cast<Rendering::LightType>(item_current_idx);

								dirty = true;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
					}

					// Edit Light Diffuse Color
					dirty |= ImGui::ColorEdit3("Diffuse", (float*)&light.color);

					// Edit Light Ambient Intensity
					dirty |= ImGui::DragFloat("Ambient Intensity", &light.ambientIntensity);

					// Edit Light Specular Intensity
					dirty |= ImGui::DragFloat("Specular Intensity", &light.specularIntensity);

					// Edit Light Specular Exponent
					dirty |= ImGui::DragInt("Specular Exponent", &light.specularExponent);

					if (light.type == Rendering::LightType::DIRECTIONAL || light.type == Rendering::LightType::SPOT)
					{
						dirty |= ImGui::DragFloat3("Direction", (float*)&light.direction, 0.005f, -1.0f, 1.0f);
					}

					if (light.type == Rendering::LightType::POINT || light.type == Rendering::LightType::SPOT)
					{
						dirty |= ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, .01f, .01f, 1.f, "%.4f");

						dirty |= ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, .01f, .01f, 2.f, "%.6f");
					}

					if (light.type == Rendering::LightType::SPOT)
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
					ecsWorld->SetComponentFlag<Rendering::LightComponent, FlagDirty>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawShadowcasterUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Rendering::ShadowCasterComponent>(m_entity))
			{
				auto& shadowcaster = ecsWorld->GetComponent<Rendering::ShadowCasterComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Shadow Caster"))
					{
						ecsWorld->SetComponentFlag<Rendering::ShadowCasterComponent, FlagDeleted>(m_entity, true);
						sceneChanged = true;
					}

					int item_current_idx = 0;
					for (int i = 0; i < Rendering::SHADOW_RESOLUTION_VALUES.size(); i++)
					{
						if (Rendering::SHADOW_RESOLUTION_VALUES[i] == shadowcaster.shadowmapWidth)
						{
							item_current_idx = i;
							break;
						}
					}

					const char* label = Rendering::SHADOW_RESOLUTION_LABELS[item_current_idx].c_str();
					if (ImGui::BeginCombo("Shadow Resolution", label))
					{
						for (int i = 0; i < Rendering::SHADOW_RESOLUTION_VALUES.size(); i++)
						{
							const bool is_selected = (item_current_idx == i);

							if (ImGui::Selectable(Rendering::SHADOW_RESOLUTION_LABELS[i].c_str(), is_selected))
							{
								item_current_idx = i;

								shadowcaster.shadowmapWidth = Rendering::SHADOW_RESOLUTION_VALUES[item_current_idx];
								shadowcaster.shadowmapHeight = Rendering::SHADOW_RESOLUTION_VALUES[item_current_idx];

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
					ecsWorld->SetComponentFlag<Rendering::ShadowCasterComponent, FlagDirty>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawProceduralPlaneUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Procedural::PlaneComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Procedural Plane Component"), flags)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& plane = ecsWorld->GetComponent<Procedural::PlaneComponent>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##ProceduralPlane"))
					{
						ecsWorld->SetComponentFlag<Procedural::PlaneComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat2("Half Size", (float*)&plane.halfSize, 0.1f);
					dirty |= ImGui::DragInt2("Num Quads", (int*)&plane.numQuads);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Procedural::PlaneComponent, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawRigidbody2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::RigidbodyComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Rigidbody Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& rb = ecsWorld->GetComponent<Physics::RigidbodyComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Rigidbody"))
					{
						ecsWorld->SetComponentFlag<Physics::RigidbodyComponent2D, FlagDeleted>(m_entity, true);
							
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
								rb.bodyType = static_cast<Physics::BodyType>(item_current_idx);
								dirty = true;
							}

							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}

						ImGui::EndCombo();
					}

					dirty |= ImGui::DragFloat("Inverse Mass", &rb.invMass);
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
						ecsWorld->SetComponentFlag<Physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawCircle2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::CircleComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Circle Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& circle = ecsWorld->GetComponent<Physics::CircleComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Circle2D"))
					{
						ecsWorld->SetComponentFlag<Physics::CircleComponent2D, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat("Radius", &circle.radius);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawBox2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::BoxComponent2D>(m_entity))
			{
				if (ImGui::TreeNodeEx("Box Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& box = ecsWorld->GetComponent<Physics::BoxComponent2D>(m_entity);
					bool dirty = false;

					if (ImGui::SmallButton("X##Box2D"))
					{
						ecsWorld->SetComponentFlag< Physics::BoxComponent2D, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					dirty |= ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&box.halfExtent), 0.1f, 0.0f);

					if (dirty)
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Physics::RigidbodyComponent2D, FlagDirty>(m_entity, true);
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawScriptUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Scripting::AngelScriptComponent>(m_entity))
			{
				Scripting::AngelScriptComponent& comp = ecsWorld->GetComponent<Scripting::AngelScriptComponent>(m_entity);

				if (ImGui::TreeNodeEx("Script Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Script"))
					{
						ecsWorld->SetComponentFlag<Scripting::AngelScriptComponent, FlagDeleted>(m_entity, true);
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