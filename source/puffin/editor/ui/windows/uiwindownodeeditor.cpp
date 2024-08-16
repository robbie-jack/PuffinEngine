#include "puffin/editor/ui/windows/uiwindownodeeditor.h"

#include <string>

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/mathhelpers.h"
#include "misc/cpp/imgui_stdlib.h"
#include "puffin/scene/scenegraph.h"
#include "puffin/nodes/node.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/rendering/meshcomponent.h"
#include "puffin/components/rendering/lightcomponent.h"
#include "puffin/components/rendering/cameracomponent.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/shapecomponent3d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/scripting/angelscriptcomponent.h"
#include "puffin/components/procedural/proceduralmeshcomponent.h"
#include "puffin/nodes/transformnode3d.h"

namespace puffin
{
	namespace ui
	{
		UIWindowNodeEditor::UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine)
		{
			add_component_type<TransformComponent3D>("Transform3D");
			add_component_type<rendering::MeshComponent>("Static Mesh");
			add_component_type<rendering::LightComponent3D>("Light 3D");
			add_component_type<rendering::ShadowCasterComponent>("Shadow Caster");
			add_component_type<physics::RigidbodyComponent3D>("Rigidbody3D");
			add_component_type<physics::BoxComponent3D>("Box3D");
			add_component_type<physics::SphereComponent3D>("Sphere3D");
			add_component_type<scripting::AngelScriptComponent>("Script");
		}

		void UIWindowNodeEditor::draw(double dt)
		{
			mWindowName = "Entity Properties";

			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			auto scene_graph_subsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

			const auto registry = entt_subsystem->registry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				if (mSelectedEntity != gInvalidID && entt_subsystem->valid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = entt_subsystem->get_entity(mSelectedEntity);
					auto node = scene_graph_subsystem->get_node_ptr(mSelectedEntity);

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

						m_scene_changed = false;

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

							draw_mesh_ui(flags, entity, mesh);
						}

						if (registry->any_of<rendering::LightComponent3D>(entity))
						{
							auto& light = registry->get<rendering::LightComponent3D>(entity);

							draw_light_ui(flags, entity, light);
						}

						if (registry->any_of<rendering::ShadowCasterComponent>(entity))
						{
							auto& shadowcaster = registry->get<rendering::ShadowCasterComponent>(entity);

							draw_shadowcaster_ui(flags, entity, shadowcaster);
						}

						if (registry->any_of<procedural::ProceduralPlaneComponent>(entity))
						{
							auto& plane = registry->get<procedural::ProceduralPlaneComponent>(entity);

							draw_procedural_plane_ui(flags, entity, plane);
						}

						if (registry->any_of<physics::RigidbodyComponent2D>(entity))
						{
							auto& rigidbody = registry->get<physics::RigidbodyComponent2D>(entity);

							draw_rigidbody_2d_ui(flags, entity, rigidbody);
						}

						if (registry->any_of<physics::CircleComponent2D>(entity))
						{
							auto& circle = registry->get<physics::CircleComponent2D>(entity);

							draw_circle_2d_ui(flags, entity, circle);
						}

						if (registry->any_of<physics::BoxComponent2D>(entity))
						{
							auto& box = registry->get<physics::BoxComponent2D>(entity);

							draw_box_2d_ui(flags, entity, box);
						}

						if (registry->any_of<scripting::AngelScriptComponent>(entity))
						{
							auto& script = registry->get<scripting::AngelScriptComponent>(entity);

							draw_script_ui(flags, entity, script);
						}

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
						for (auto& handler : m_component_handlers)
						{
							if (ImGui::Selectable(handler->name().c_str()))
							{
								if (handler->add(node))
								{
									m_scene_changed = true;
								}
							}
						}

						ImGui::EndPopup();
					}
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
                const auto transform = node->transform_2d();

				{
#ifdef PFN_DOUBLE_PRECISION

                    Vector2d position = transform->position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 2, 0.1f))
					{
                        node->transform_2d()->position = position;

						mSceneChanged = true;
					}

#else

                    Vector2f position = transform->position;

					if (ImGui::DragFloat2("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
                        node->transform_2d()->position = position;

						m_scene_changed = true;
					}

#endif
				}

				{
                    float rotation = transform->rotation;

					if (ImGui::DragFloat("Rotation", &rotation, 0.1f, -180.0f, 180.0f))
					{
                        node->transform_2d()->rotation = rotation;

						m_scene_changed = true;
					}
				}

				{
                    Vector2f scale = transform->scale;

					if (ImGui::DragFloat2("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
                        node->transform_2d()->scale = scale;

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_transform_ui_3d_node(ImGuiTreeNodeFlags flags, Node* node)
		{
			if (ImGui::TreeNodeEx("Transform", flags))
			{
				auto node_3d = static_cast<TransformNode3D*>(node);
                auto transform = node->transform_3d();

				{
#ifdef PFN_USE_DOUBLE_PRECISION

					Vector3d position = transform.position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 3, 0.1f))
					{
						node->transform_3d().position = position;

						mSceneChanged = true;
					}

#else

                    Vector3f position = transform->position;

					if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
                        node->transform_3d()->position = position;

						m_scene_changed = true;
					}

#endif
				}

				{
					auto euler_angles = transform->orientationEulerAngles;

					if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&euler_angles), 0.2f, 0, 0, "%.3f"))
					{
						UpdateTransformOrientation(*node->transform_3d(), euler_angles);

						m_scene_changed = true;
					}
				}

				{
                    Vector3f scale = transform->scale;

					if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
                        node->transform_3d()->scale = scale;

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_mesh_ui(const ImGuiTreeNodeFlags flags, const entt::entity entity, rendering::MeshComponent& mesh)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Mesh Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Mesh"))
				{
					entt_subsystem->registry()->remove<rendering::MeshComponent>(entity);

					m_scene_changed = true;
				}

                ImGui::Text("Model UUID: %lu", mesh.mesh_asset_id);
                ImGui::Text("Material UUID: %lu", mesh.mat_asset_id);

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

		void UIWindowNodeEditor::draw_light_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent3D& light)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Light Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Point Light"))
				{
					registry->remove<rendering::LightComponent3D>(entity);

					m_scene_changed = true;
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
							registry->patch<rendering::LightComponent3D>(entity, [&lightType](auto& light)
							{
								light.type = lightType;
							});

							m_scene_changed = true;
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
						registry->patch<rendering::LightComponent3D>(entity, [&color](auto& light) { light.color = color; });

						m_scene_changed = true;
					}
				}

				// Edit Light Ambient Intensity
				{
					float ambientIntensity = light.ambientIntensity;

					if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&ambientIntensity](auto& light) { light.ambient_intensity = ambientIntensity; });

						m_scene_changed = true;
					}
				}

				// Edit Light Specular Intensity
				{
					float specular_intensity = light.specularIntensity;

					if (ImGui::DragFloat("Specular Intensity", &specular_intensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&specular_intensity](auto& light) { light.specular_intensity = specular_intensity; });

						m_scene_changed = true;
					}
				}

				// Edit Light Specular Exponent
				{
					int specular_exponent = light.specularExponent;

					if (ImGui::DragInt("Specular Exponent", &specular_exponent, 0.1f, 1, 128))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&specular_exponent](auto& light) { light.specular_exponent = specular_exponent; });

						m_scene_changed = true;
					}
				}

				if (light.type == rendering::LightType::Point || light.type == rendering::LightType::Spot)
				{
					{
						float linear_attenuation = light.linearAttenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linear_attenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::LightComponent3D>(entity, [&linear_attenuation](auto& light) { light.linear_attenuation = linear_attenuation; });

							m_scene_changed = true;
						}
					}

					{
						float quadratic_attenuation = light.quadraticAttenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadratic_attenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::LightComponent3D>(entity, [&quadratic_attenuation](auto& light) { light.quadratic_attenuation = quadratic_attenuation; });

							m_scene_changed = true;
						}
					}
				}

				if (light.type == rendering::LightType::Spot)
				{
					{
						float inner_cutoff_angle = light.innerCutoffAngle;

						if (ImGui::DragFloat("Inner Cutoff Angle", &inner_cutoff_angle, 0.25f, 0.0f))
						{
							if (inner_cutoff_angle > 90.0f)
								inner_cutoff_angle = 90.0f;

							registry->patch<rendering::LightComponent3D>(entity, [&inner_cutoff_angle](auto& light) { light.inner_cutoff_angle = inner_cutoff_angle; });

							m_scene_changed = true;
						}
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					{
						float outer_cutoff_angle = light.outerCutoffAngle;

						if (ImGui::DragFloat("Outer Cutoff Angle", &outer_cutoff_angle, 0.25f, light.outerCutoffAngle))
						{
							if (outer_cutoff_angle > 90.0f)
								outer_cutoff_angle = 90.0f;

							registry->patch<rendering::LightComponent3D>(entity, [&outer_cutoff_angle](auto& light) { light.outer_cutoff_angle = outer_cutoff_angle; });

							m_scene_changed = true;
						}

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (light.outerCutoffAngle < light.innerCutoffAngle)
						{
							light.outerCutoffAngle = light.innerCutoffAngle;

							m_scene_changed = true;
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_shadowcaster_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadow)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					entt_subsystem->registry()->remove<rendering::ShadowCasterComponent>(entity);

					m_scene_changed = true;
				}

				{
					int item_current_idx = 0;
					for (int i = 0; i < rendering::g_shadow_resolution_values.size(); i++)
					{
						if (rendering::g_shadow_resolution_values[i] == shadow.width)
						{
							item_current_idx = i;
							break;
						}
					}

					if (const char* label = rendering::g_shadow_resolution_labels[item_current_idx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
					{
						for (int i = 0; i < rendering::g_shadow_resolution_values.size(); i++)
						{
							const bool selected = (item_current_idx == i);

							if (ImGui::Selectable(rendering::g_shadow_resolution_labels[i].c_str(), selected))
							{
								item_current_idx = i;

								uint16_t width = rendering::g_shadow_resolution_values[item_current_idx];
								uint16_t height = width;

								registry->patch<rendering::ShadowCasterComponent>(entity, [&width, &height](auto& shadow)
								{
									shadow.width = width;
									shadow.height = height;
								});

								m_scene_changed = true;
							}

							if (selected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}
				}

				{
					float bias_min = shadow.bias_min;

					if (ImGui::DragFloat("Bias Min", &bias_min, 0.001f))
					{
						registry->patch<rendering::ShadowCasterComponent>(entity, [&bias_min](auto& shadow)
						{
							shadow.bias_min = bias_min;
						});

						m_scene_changed = true;
					}
				}

				{
					float bias_max = shadow.bias_max;

					if (ImGui::DragFloat("Bias Max", &bias_max, 0.001f))
					{
						registry->patch<rendering::ShadowCasterComponent>(entity, [&bias_max](auto& shadow)
						{
							shadow.bias_max = bias_max;
						});

						m_scene_changed = true;
					}
				}

				{
					float bounds_mult = shadow.bounds_mult;

					if (ImGui::DragFloat("Bounds Multiplier", &bounds_mult, 0.1f))
					{
						registry->patch<rendering::ShadowCasterComponent>(entity, [&bounds_mult](auto& shadow)
						{
							shadow.bounds_mult = bounds_mult;
						});

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_procedural_plane_ui(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::ProceduralPlaneComponent& plane)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Procedural Plane Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##ProceduralPlane"))
				{
					registry->remove<procedural::ProceduralPlaneComponent>(entity);

					m_scene_changed = true;
				}

				{
					Vector2f halfSize = plane.halfSize;

					if (ImGui::DragFloat2("Half Size", reinterpret_cast<float*>(&halfSize), 0.1f))
					{
						registry->patch<procedural::ProceduralPlaneComponent>(entity, [&halfSize](auto& plane) { plane.halfSize = halfSize; });

						m_scene_changed = true;
					}
				}

				{
					Vector2i quadCount = plane.quadCount;

					if (ImGui::DragInt2("Num Quads", reinterpret_cast<int*>(&quadCount)))
					{
						registry->patch<procedural::ProceduralPlaneComponent>(entity, [&quadCount](auto& plane) { plane.quadCount = quadCount; });

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_rigidbody_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Rigidbody Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Rigidbody"))
				{
					registry->remove<physics::RigidbodyComponent2D>(entity);
						
					m_scene_changed = true;
				}

				// Combo box for Body Type Selection
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				int item_current_idx = static_cast<int>(rigidbody.bodyType);

				if (const char* label = items[item_current_idx]; ImGui::BeginCombo("Body Type", label))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); i++)
					{
						const bool is_selected = (item_current_idx == i);
						if (ImGui::Selectable(items[i], is_selected))
						{
							item_current_idx = i;

							const auto bodyType = static_cast<physics::BodyType>(item_current_idx);

							registry->patch<physics::RigidbodyComponent2D>(entity, [&bodyType](auto& rigidbody)
							{
								rigidbody.bodyType = bodyType;
							});

							m_scene_changed = true;
						}

						if (is_selected)
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

						m_scene_changed = true;
					}
				}

				{
					float elasticity = rigidbody.elasticity;

					if (ImGui::DragFloat("Elasticity", &elasticity))
					{
						registry->patch<physics::RigidbodyComponent2D>(entity, [&elasticity](auto& rigidbody) { rigidbody.elasticity = elasticity; });

						m_scene_changed = true;
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

		void UIWindowNodeEditor::draw_circle_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Circle Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Circle2D"))
				{
					registry->remove<physics::CircleComponent2D>(entity);

					m_scene_changed = true;
				}

				{
					Vector2f centreOfMass = circle.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&centreOfMass](auto& circle) { circle.centreOfMass = centreOfMass; });

						m_scene_changed = true;
					}
				}

				{
					float radius = circle.radius;

					if (ImGui::DragFloat("Radius", &radius))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&radius](auto& circle) { circle.radius = radius; });

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_box_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Box Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Box2D"))
				{
					registry->remove<physics::BoxComponent2D>(entity);					

					m_scene_changed = true;
				}

				{
					Vector2f centreOfMass = box.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&centreOfMass](auto& box) { box.centreOfMass = centreOfMass; });

						m_scene_changed = true;
					}
				}

				{
					Vector2f halfExtent = box.halfExtent;

					if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&halfExtent), 0.1f, 0.0f))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&halfExtent](auto& box) { box.halfExtent = halfExtent; });

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_script_ui(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Script Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Script"))
				{
					registry->remove<scripting::AngelScriptComponent>(entity);
					
					m_scene_changed = true;
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
