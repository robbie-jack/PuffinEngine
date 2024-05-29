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
		UIWindowNodeEditor::UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine)
		{
			add_component_type<TransformComponent3D>("Transform3D");
			add_component_type<rendering::MeshComponent>("Static Mesh");
			add_component_type<rendering::LightComponent>("Light 3D");
			add_component_type<rendering::ShadowCasterComponent>("Shadow Caster");
			add_component_type<physics::RigidbodyComponent3D>("Rigidbody3D");
			add_component_type<physics::BoxComponent3D>("Box3D");
			add_component_type<physics::SphereComponent3D>("Sphere3D");
			add_component_type<scripting::AngelScriptComponent>("Script");
		}

		void UIWindowNodeEditor::draw(double dt)
		{
			mWindowName = "Entity Properties";

			if (!m_entt_subsystem)
				m_entt_subsystem = mEngine->get_system<ecs::EnTTSubsystem>();

			if (!m_scene_graph)
				m_scene_graph = mEngine->get_system<scene::SceneGraph>();

			const auto registry = m_entt_subsystem->registry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				if (mSelectedEntity != gInvalidID && m_entt_subsystem->valid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = m_entt_subsystem->get_entity(mSelectedEntity);
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

						if (registry->any_of<rendering::LightComponent>(entity))
						{
							auto& light = registry->get<rendering::LightComponent>(entity);

							draw_light_ui(flags, entity, light);
						}

						if (registry->any_of<rendering::ShadowCasterComponent>(entity))
						{
							auto& shadowcaster = registry->get<rendering::ShadowCasterComponent>(entity);

							draw_shadowcaster_ui(flags, entity, shadowcaster);
						}

						if (registry->any_of<procedural::PlaneComponent>(entity))
						{
							auto& plane = registry->get<procedural::PlaneComponent>(entity);

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
                const auto transform = node->transform_3d();

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
                    Vector3f anglesDeg = maths::radToDeg(eulerAngles(static_cast<glm::quat>(transform->orientation)));

					if (ImGui::DragFloat3("Orientation", reinterpret_cast<float*>(&anglesDeg), 0.2f, -180.f, 180.f, "%.3f"))
					{
						const Vector3f angles_rad = maths::degToRad(anglesDeg);

						const glm::quat quat_x(angleAxis(angles_rad.x, glm::vec3(1.f, 0.f, 0.f)));
						const glm::quat quat_y(angleAxis(angles_rad.y, glm::vec3(0.f, 1.f, 0.f)));
						const glm::quat quat_z(angleAxis(angles_rad.z, glm::vec3(0.f, 0.f, 1.f)));

                        node->transform_3d()->orientation = quat_y * quat_x * quat_z;

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
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Mesh Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Mesh"))
				{
					m_entt_subsystem->registry()->remove<rendering::MeshComponent>(entity);

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

		void UIWindowNodeEditor::draw_light_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light)
		{
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Light Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Point Light"))
				{
					m_entt_subsystem->registry()->remove<rendering::LightComponent>(entity);

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
							registry->patch<rendering::LightComponent>(entity, [&lightType](auto& light)
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
						registry->patch<rendering::LightComponent>(entity, [&color](auto& light) { light.color = color; });

						m_scene_changed = true;
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

							m_scene_changed = true;
						}
					}
				}

				// Edit Light Ambient Intensity
				{
					float ambientIntensity = light.ambient_intensity;

					if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&ambientIntensity](auto& light) { light.ambient_intensity = ambientIntensity; });

						m_scene_changed = true;
					}
				}

				// Edit Light Specular Intensity
				{
					float specular_intensity = light.specular_intensity;

					if (ImGui::DragFloat("Specular Intensity", &specular_intensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&specular_intensity](auto& light) { light.specular_intensity = specular_intensity; });

						m_scene_changed = true;
					}
				}

				// Edit Light Specular Exponent
				{
					int specular_exponent = light.specular_exponent;

					if (ImGui::DragInt("Specular Exponent", &specular_exponent, 0.1f, 1, 128))
					{
						registry->patch<rendering::LightComponent>(entity, [&specular_exponent](auto& light) { light.specular_exponent = specular_exponent; });

						m_scene_changed = true;
					}
				}

				if (light.type == rendering::LightType::Point || light.type == rendering::LightType::Spot)
				{
					{
						float linear_attenuation = light.linear_attenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linear_attenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&linear_attenuation](auto& light) { light.linear_attenuation = linear_attenuation; });

							m_scene_changed = true;
						}
					}

					{
						float quadratic_attenuation = light.quadratic_attenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadratic_attenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&quadratic_attenuation](auto& light) { light.quadratic_attenuation = quadratic_attenuation; });

							m_scene_changed = true;
						}
					}
				}

				if (light.type == rendering::LightType::Spot)
				{
					{
						float inner_cutoff_angle = light.inner_cutoff_angle;

						if (ImGui::DragFloat("Inner Cutoff Angle", &inner_cutoff_angle, 0.25f, 0.0f))
						{
							if (inner_cutoff_angle > 90.0f)
								inner_cutoff_angle = 90.0f;

							registry->patch<rendering::LightComponent>(entity, [&inner_cutoff_angle](auto& light) { light.inner_cutoff_angle = inner_cutoff_angle; });

							m_scene_changed = true;
						}
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					{
						float outer_cutoff_angle = light.outer_cutoff_angle;

						if (ImGui::DragFloat("Outer Cutoff Angle", &outer_cutoff_angle, 0.25f, light.outer_cutoff_angle))
						{
							if (outer_cutoff_angle > 90.0f)
								outer_cutoff_angle = 90.0f;

							registry->patch<rendering::LightComponent>(entity, [&outer_cutoff_angle](auto& light) { light.outer_cutoff_angle = outer_cutoff_angle; });

							m_scene_changed = true;
						}

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (light.outer_cutoff_angle < light.inner_cutoff_angle)
						{
							light.outer_cutoff_angle = light.inner_cutoff_angle;

							m_scene_changed = true;
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_shadowcaster_ui(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadowcaster)
		{
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					m_entt_subsystem->registry()->remove<rendering::ShadowCasterComponent>(entity);

					m_scene_changed = true;
				}

				int itemCurrentIdx = 0;
				for (int i = 0; i < rendering::g_shadow_resolution_values.size(); i++)
				{
					if (rendering::g_shadow_resolution_values[i] == shadowcaster.width)
					{
						itemCurrentIdx = i;
						break;
					}
				}

				if (const char* label = rendering::g_shadow_resolution_labels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
				{
					for (int i = 0; i < rendering::g_shadow_resolution_values.size(); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);

						if (ImGui::Selectable(rendering::g_shadow_resolution_labels[i].c_str(), isSelected))
						{
							itemCurrentIdx = i;

							uint16_t width = rendering::g_shadow_resolution_values[itemCurrentIdx];
							uint16_t height = width;
							
							registry->patch<rendering::ShadowCasterComponent>(entity, [&width, &height](auto& shadowcaster)
							{
								shadowcaster.width = width;
								shadowcaster.height = height;
							});

							m_scene_changed = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_procedural_plane_ui(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::PlaneComponent& plane)
		{
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Procedural Plane Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##ProceduralPlane"))
				{
					m_entt_subsystem->registry()->remove<procedural::PlaneComponent>(entity);

					m_scene_changed = true;
				}

				{
					Vector2f half_size = plane.half_size;

					if (ImGui::DragFloat2("Half Size", reinterpret_cast<float*>(&half_size), 0.1f))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&half_size](auto& plane) { plane.half_size = half_size; });

						m_scene_changed = true;
					}
				}

				{
					Vector2i num_quads = plane.num_quads;

					if (ImGui::DragInt2("Num Quads", reinterpret_cast<int*>(&num_quads)))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&num_quads](auto& plane) { plane.num_quads = num_quads; });

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_rigidbody_2d_ui(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		{
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Rigidbody Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Rigidbody"))
				{
					m_entt_subsystem->registry()->remove<physics::RigidbodyComponent2D>(entity);
						
					m_scene_changed = true;
				}

				// Combo box for Body Type Selection
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				int item_current_idx = static_cast<int>(rigidbody.body_type);

				if (const char* label = items[item_current_idx]; ImGui::BeginCombo("Body Type", label))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); i++)
					{
						const bool is_selected = (item_current_idx == i);
						if (ImGui::Selectable(items[i], is_selected))
						{
							item_current_idx = i;

							const auto body_type = static_cast<physics::BodyType>(item_current_idx);

							registry->patch<physics::RigidbodyComponent2D>(entity, [&body_type](auto& rigidbody)
							{
								rigidbody.body_type = body_type;
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
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Circle Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Circle2D"))
				{
					m_entt_subsystem->registry()->remove<physics::CircleComponent2D>(entity);

					m_scene_changed = true;
				}

				{
					Vector2f centre_of_mass = circle.centre_of_mass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centre_of_mass)))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&centre_of_mass](auto& circle) { circle.centre_of_mass = centre_of_mass; });

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
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Box Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Box2D"))
				{
					m_entt_subsystem->registry()->remove<physics::BoxComponent2D>(entity);					

					m_scene_changed = true;
				}

				{
					Vector2f centre_of_mass = box.centre_of_mass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centre_of_mass)))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&centre_of_mass](auto& box) { box.centre_of_mass = centre_of_mass; });

						m_scene_changed = true;
					}
				}

				{
					Vector2f half_extent = box.half_extent;

					if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&half_extent), 0.1f, 0.0f))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&half_extent](auto& box) { box.half_extent = half_extent; });

						m_scene_changed = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::draw_script_ui(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script)
		{
			const auto registry = m_entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Script Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Script"))
				{
					m_entt_subsystem->registry()->remove<scripting::AngelScriptComponent>(entity);
					
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
