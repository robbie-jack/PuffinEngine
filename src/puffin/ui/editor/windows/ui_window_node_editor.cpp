#include "puffin/ui/editor/windows/ui_window_node_editor.h"

#include <string>

#include "puffin/core/engine.h"
#include "puffin/ecs/entt_subsystem.h"
#include "puffin/math_helpers.h"
#include "misc/cpp/imgui_stdlib.h"
#include "puffin/scene/scene_graph.h"
#include "puffin/nodes/node.h"
#include "puffin/components/transform_component_2d.h"
#include "puffin/components/transform_component_3d.h"
#include "puffin/components/rendering/mesh_component.h"
#include "puffin/components/rendering/light_component.h"
#include "puffin/components/rendering/camera_component.h"
#include "puffin/components/physics/2d/rigidbody_component_2d.h"
#include "puffin/components/physics/2d/shape_components_2d.h"
#include "puffin/components/physics/2d/velocity_component_2d.h"
#include "puffin/components/physics/3d/rigidbody_component_3d.h"
#include "puffin/components/physics/3d/shape_components_3d.h"
#include "puffin/components/physics/3d/velocity_component_3d.h"
#include "puffin/components/scripting/angelscript_component.h"
#include "puffin/components/procedural/procedural_mesh_component.h"

namespace puffin
{
	namespace ui
	{
		void UIWindowNodeEditor::draw(double dt)
		{
			mWindowName = "Entity Properties";

			if (!mEnTTSubsystem)
				mEnTTSubsystem = mEngine->getSystem<ecs::EnTTSubsystem>();

			if (!m_scene_graph)
				m_scene_graph = mEngine->getSystem<scene::SceneGraph>();

			const auto registry = mEnTTSubsystem->registry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				if (mSelectedEntity != gInvalidID && mEnTTSubsystem->valid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = mEnTTSubsystem->get_entity(mSelectedEntity);
					auto node = m_scene_graph->get_node_ptr(mSelectedEntity);

					// Edit Entity Name
					std::string name = node->name();
					if (std::string* namePtr = &name; ImGui::InputText("##name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						node->set_name(*namePtr);
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

						mSceneChanged = false;

						// Display Component UI
						if (node->has_transform_2d())
						{
							draw_transform_ui_2d_node(flags, node);
						}

						if (node->has_transform_3d())
						{
							draw_transform_ui_3d_node(flags, node);
						}

						if (registry->any_of<rendering::MeshComponent>(entity))
						{
							auto& mesh = registry->get<rendering::MeshComponent>(entity);

							drawMeshUI(flags, entity, mesh);
						}

						if (registry->any_of<rendering::LightComponent>(entity))
						{
							auto& light = registry->get<rendering::LightComponent>(entity);

							drawLightUI(flags, entity, light);
						}

						if (registry->any_of<rendering::ShadowCasterComponent>(entity))
						{
							auto& shadowcaster = registry->get<rendering::ShadowCasterComponent>(entity);

							drawShadowcasterUI(flags, entity, shadowcaster);
						}

						if (registry->any_of<procedural::PlaneComponent>(entity))
						{
							auto& plane = registry->get<procedural::PlaneComponent>(entity);

							drawProceduralPlaneUI(flags, entity, plane);
						}

						if (registry->any_of<physics::RigidbodyComponent2D>(entity))
						{
							auto& rigidbody = registry->get<physics::RigidbodyComponent2D>(entity);

							drawRigidbody2DUI(flags, entity, rigidbody);
						}

						if (registry->any_of<physics::CircleComponent2D>(entity))
						{
							auto& circle = registry->get<physics::CircleComponent2D>(entity);

							drawCircle2DUI(flags, entity, circle);
						}

						if (registry->any_of<physics::BoxComponent2D>(entity))
						{
							auto& box = registry->get<physics::BoxComponent2D>(entity);

							drawBox2DUI(flags, entity, box);
						}

						if (registry->any_of<scripting::AngelScriptComponent>(entity))
						{
							auto& script = registry->get<scripting::AngelScriptComponent>(entity);

							drawScriptUI(flags, entity, script);
						}

						ImGui::EndListBox();
					}

					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

					if (ImGui::Button("Add Component"))
					{
						ImGui::OpenPopup("Add Component");
					}

					// Display Add Component Popup
					/*if (ImGui::BeginPopup("Add Component"))
					{
						if (ImGui::Selectable("Transform"))
						{
							if (!ecsWorld->HasComponent<TransformComponent3D>(m_entity))
							{
								ecsWorld->AddComponent<TransformComponent3D>(m_entity);
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
					}*/
				}
				else
				{
					ImGui::Text(" No Entity Selected");
				}

				end();
			}
		}

		void UIWindowNodeEditor::draw_transform_ui_2d_node(ImGuiTreeNodeFlags flags, Node* node)
		{
			if (ImGui::TreeNodeEx("Transform", flags))
			{
				const TransformComponent2D& transform = node->transform_2d();

				{
#ifdef PFN_DOUBLE_PRECISION

					Vector2d position = transform.position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 2, 0.1f))
					{
						node->transform_2d().position = position;

						mSceneChanged = true;
					}

#else

					Vector2f position = transform.position;

					if (ImGui::DragFloat2("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
						node->transform_2d().position = position;

						mSceneChanged = true;
					}

#endif
				}

				{
					float rotation = transform.rotation;

					if (ImGui::DragFloat("Rotation", &rotation, 0.1f, -180.0f, 180.0f))
					{
						node->transform_2d().rotation = rotation;

						mSceneChanged = true;
					}
				}

				{
					Vector2f scale = transform.scale;

					if (ImGui::DragFloat2("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
						node->transform_2d().scale = scale;

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_transform_ui_3d_node(ImGuiTreeNodeFlags flags, Node* node)
		{
			if (ImGui::TreeNodeEx("Transform", flags))
			{
				const TransformComponent3D& transform = node->transform_3d();

				{
#ifdef PFN_USE_DOUBLE_PRECISION

					Vector3d position = transform.position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 3, 0.1f))
					{
						node->transform_3d().position = position;

						mSceneChanged = true;
					}

#else

					Vector3f position = transform.position;

					if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
						node->transform_3d().position = position;

						mSceneChanged = true;
					}

#endif
				}

				{
					Vector3f anglesDeg = maths::radToDeg(eulerAngles(static_cast<glm::quat>(transform.orientation)));

					if (ImGui::DragFloat3("Orientation", reinterpret_cast<float*>(&anglesDeg), 0.2f, -180.f, 180.f, "%.3f"))
					{
						const Vector3f angles_rad = maths::degToRad(anglesDeg);

						const glm::quat quat_x(angleAxis(angles_rad.x, glm::vec3(1.f, 0.f, 0.f)));
						const glm::quat quat_y(angleAxis(angles_rad.y, glm::vec3(0.f, 1.f, 0.f)));
						const glm::quat quat_z(angleAxis(angles_rad.z, glm::vec3(0.f, 0.f, 1.f)));

						node->transform_3d().orientation = quat_y * quat_x * quat_z;

						mSceneChanged = true;
					}
				}

				{
					Vector3f scale = transform.scale;

					if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
						node->transform_3d().scale = scale;

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawMeshUI(const ImGuiTreeNodeFlags flags, const entt::entity entity, rendering::MeshComponent& mesh)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Mesh Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Mesh"))
				{
					mEnTTSubsystem->registry()->remove<rendering::MeshComponent>(entity);

					mSceneChanged = true;
				}

				ImGui::Text("Model UUID: %llu", mesh.meshAssetID);
				ImGui::Text("Material UUID: %llu", mesh.matAssetID);

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
					mSceneChanged = true;
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

		void UIWindowNodeEditor::drawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Light Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Point Light"))
				{
					mEnTTSubsystem->registry()->remove<rendering::LightComponent>(entity);

					mSceneChanged = true;
				}

				int itemCurrentIdx = static_cast<int>(light.type);

				// Edit Light Type
				if (const char* comboLabel = rendering::gLightTypeLabels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Type", comboLabel))
				{
					for (int i = 0; i < rendering::gLightTypeLabels.size(); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);

						if (ImGui::Selectable(rendering::gLightTypeLabels[i].c_str(), isSelected))
						{
							itemCurrentIdx = i;

							const auto lightType = static_cast<rendering::LightType>(itemCurrentIdx);
							registry->patch<rendering::LightComponent>(entity, [&lightType](auto& light)
							{
								light.type = lightType;
							});

							mSceneChanged = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				// Edit Light Diffuse Color
				{
					Vector3f color = light.color;

					if (ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&color)))
					{
						registry->patch<rendering::LightComponent>(entity, [&color](auto& light) { light.color = color; });

						mSceneChanged = true;
					}
				}

				// Edit light direction
				if (light.type == rendering::LightType::Directional || light.type == rendering::LightType::Spot)
				{
					{
						Vector3f direction = light.direction;

						if (ImGui::DragFloat3("Direction", reinterpret_cast<float*>(&direction), 0.01f, -1.0f, 1.0f))
						{
							direction.normalize();

							registry->patch<rendering::LightComponent>(entity, [&direction](auto& light) { light.direction = direction; });

							mSceneChanged = true;
						}
					}
				}

				// Edit Light Ambient Intensity
				{
					float ambientIntensity = light.ambientIntensity;

					if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Intensity
				{
					float specularIntensity = light.specularIntensity;

					if (ImGui::DragFloat("Specular Intensity", &specularIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Exponent
				{
					int specularExponent = light.specularExponent;

					if (ImGui::DragInt("Specular Exponent", &specularExponent, 0.1f, 1, 128))
					{
						registry->patch<rendering::LightComponent>(entity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });

						mSceneChanged = true;
					}
				}

				if (light.type == rendering::LightType::Point || light.type == rendering::LightType::Spot)
				{
					{
						float linearAttenuation = light.linearAttenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });

							mSceneChanged = true;
						}
					}

					{
						float quadraticAttenuation = light.quadraticAttenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });

							mSceneChanged = true;
						}
					}
				}

				if (light.type == rendering::LightType::Spot)
				{
					{
						float innerCutoffAngle = light.innerCutoffAngle;

						if (ImGui::DragFloat("Inner Cutoff Angle", &innerCutoffAngle, 0.25f, 0.0f, 45.0f))
						{
							registry->patch<rendering::LightComponent>(entity, [&innerCutoffAngle](auto& light) { light.innerCutoffAngle = innerCutoffAngle; });

							mSceneChanged = true;
						}
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					{
						float outerCutoffAngle = light.outerCutoffAngle;

						if (ImGui::DragFloat("Outer Cutoff Angle", &outerCutoffAngle, 0.25f, light.outerCutoffAngle, 45.0f))
						{
							registry->patch<rendering::LightComponent>(entity, [&outerCutoffAngle](auto& light) { light.outerCutoffAngle = outerCutoffAngle; });

							mSceneChanged = true;
						}

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (light.outerCutoffAngle < light.innerCutoffAngle)
						{
							light.outerCutoffAngle = light.innerCutoffAngle;

							mSceneChanged = true;
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawShadowcasterUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadowcaster)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					mEnTTSubsystem->registry()->remove<rendering::ShadowCasterComponent>(entity);

					mSceneChanged = true;
				}

				int itemCurrentIdx = 0;
				for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
				{
					if (rendering::gShadowResolutionValues[i] == shadowcaster.width)
					{
						itemCurrentIdx = i;
						break;
					}
				}

				if (const char* label = rendering::gShadowResolutionLabels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
				{
					for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);

						if (ImGui::Selectable(rendering::gShadowResolutionLabels[i].c_str(), isSelected))
						{
							itemCurrentIdx = i;

							uint16_t width = rendering::gShadowResolutionValues[itemCurrentIdx];
							uint16_t height = width;
							
							registry->patch<rendering::ShadowCasterComponent>(entity, [&width, &height](auto& shadowcaster)
							{
								shadowcaster.width = width;
								shadowcaster.height = height;
							});

							mSceneChanged = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::PlaneComponent& plane)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Procedural Plane Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##ProceduralPlane"))
				{
					mEnTTSubsystem->registry()->remove<procedural::PlaneComponent>(entity);

					mSceneChanged = true;
				}

				{
					Vector2f halfSize = plane.halfSize;

					if (ImGui::DragFloat2("Half Size", reinterpret_cast<float*>(&halfSize), 0.1f))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&halfSize](auto& plane) { plane.halfSize = halfSize; });

						mSceneChanged = true;
					}
				}

				{
					Vector2i numQuads = plane.numQuads;

					if (ImGui::DragInt2("Num Quads", reinterpret_cast<int*>(&numQuads)))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&numQuads](auto& plane) { plane.numQuads = numQuads; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Rigidbody Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Rigidbody"))
				{
					mEnTTSubsystem->registry()->remove<physics::RigidbodyComponent2D>(entity);
						
					mSceneChanged = true;
				}

				// Combo box for Body Type Selection
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				int itemCurrentIdx = static_cast<int>(rigidbody.bodyType);

				if (const char* label = items[itemCurrentIdx]; ImGui::BeginCombo("Body Type", label))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);
						if (ImGui::Selectable(items[i], isSelected))
						{
							itemCurrentIdx = i;

							const auto bodyType = static_cast<physics::BodyType>(itemCurrentIdx);

							registry->patch<physics::RigidbodyComponent2D>(entity, [&bodyType](auto& rigidbody)
							{
								rigidbody.bodyType = bodyType;
							});

							mSceneChanged = true;
						}

						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				{
					float mass = rigidbody.mass;

					if (ImGui::DragFloat("Mass", &mass))
					{
						registry->patch<physics::RigidbodyComponent2D>(entity, [&mass](auto& rigidbody) { rigidbody.mass = mass; });

						mSceneChanged = true;
					}
				}

				{
					float elasticity = rigidbody.elasticity;

					if (ImGui::DragFloat("Elasticity", &elasticity))
					{
						registry->patch<physics::RigidbodyComponent2D>(entity, [&elasticity](auto& rigidbody) { rigidbody.elasticity = elasticity; });

						mSceneChanged = true;
					}
				}

				//ImGui::Checkbox("Sleeping Allowed", &sleep);
				//ImGui::Checkbox("Bullet", &bullet);
				//ImGui::Checkbox("Awake", &awake);
				//ImGui::Checkbox("Rotation Fixed", &fixedRotation);
				//ImGui::DragFloat("Angular Damping", &angularDamping);
				//ImGui::DragFloat("Linear Damping", &linearDamping);
				//ImGui::DragFloat("Gravity Scale", &gravityScale);

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Circle Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Circle2D"))
				{
					mEnTTSubsystem->registry()->remove<physics::CircleComponent2D>(entity);

					mSceneChanged = true;
				}

				{
					Vector2f centreOfMass = circle.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&centreOfMass](auto& circle) { circle.centreOfMass = centreOfMass; });

						mSceneChanged = true;
					}
				}

				{
					float radius = circle.radius;

					if (ImGui::DragFloat("Radius", &radius))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&radius](auto& circle) { circle.radius = radius; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Box Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Box2D"))
				{
					mEnTTSubsystem->registry()->remove<physics::BoxComponent2D>(entity);					

					mSceneChanged = true;
				}

				{
					Vector2f centreOfMass = box.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&centreOfMass](auto& box) { box.centreOfMass = centreOfMass; });

						mSceneChanged = true;
					}
				}

				{
					Vector2f halfExtent = box.halfExtent;

					if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&halfExtent), 0.1f, 0.0f))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&halfExtent](auto& box) { box.halfExtent = halfExtent; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::drawScriptUI(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Script Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Script"))
				{
					mEnTTSubsystem->registry()->remove<scripting::AngelScriptComponent>(entity);
					
					mSceneChanged = true;
				}

				ImGui::InputText("Script Name", &script.name);

				ImGui::Text("File Path:"); ImGui::SameLine(0.0f);
				if (ImGui::Selectable(script.dir.string().c_str(), false))
				{
					//fileDialog->Open();
					//modelSelected = true;
				}

				if (script.obj != 0)
				{
					ImGui::Separator();
					ImGui::Text("Editable Properties");

					// Display Variables in scripts marked as editable
					for (const int& index : script.editableProperties)
					{
						// Get Property Name/Type
						const char* propertyName = script.obj->GetPropertyName(index);
						const int propertyType = script.obj->GetPropertyTypeId(index);

						// Display property with correct UI based on its type
						if (propertyType == asTYPEID_BOOL)
						{
							auto boolProperty = static_cast<bool*>(script.obj->GetAddressOfProperty(index));

							ImGui::Checkbox(propertyName, boolProperty);
						}
						else if (propertyType == asTYPEID_INT32)
						{
							auto intProperty = static_cast<int*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputInt(propertyName, intProperty);
						}
						else if (propertyType == asTYPEID_FLOAT)
						{
							auto* floatProperty = static_cast<float*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputFloat(propertyName, floatProperty);
						}
						else if (propertyType == asTYPEID_DOUBLE)
						{
							auto* doubleProperty = static_cast<double*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputDouble(propertyName, doubleProperty);
						}
						else
						{
							auto* stringProperty = static_cast<std::string*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputText(propertyName, stringProperty);
						}
					}

					ImGui::Separator();
					ImGui::Text("Visible Properties");

					// Display Variables in scripts marked as visible
					for (const int& index : script.visibleProperties)
					{
						// Get Property Name/Type
						const char* propertyName = script.obj->GetPropertyName(index);
						const int propertyType = script.obj->GetPropertyTypeId(index);

						std::string propertyText = propertyName;

						// Display property with correct UI based on its type
						if (propertyType == asTYPEID_INT32)
						{
							const int intProperty = *static_cast<int*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %d";

							ImGui::Text(propertyText.c_str(), intProperty);
						}
						else if (propertyType == asTYPEID_FLOAT)
						{
							const float floatProperty = *static_cast<float*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %.20f";

							ImGui::Text(propertyText.c_str(), floatProperty);
						}
						else if (propertyType == asTYPEID_DOUBLE)
						{
							const double doubleProperty = *static_cast<double*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %.20f";

							ImGui::Text(propertyText.c_str(), doubleProperty);
						}
						else
						{
							std::string stringProperty = *static_cast<std::string*>(script.obj->GetAddressOfProperty(index));
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
