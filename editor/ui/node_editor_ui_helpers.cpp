#include "ui/node_editor_ui_helpers.h"

#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"
#include "puffin/nodes/rendering/3d/lightnode3d.h"
#include "puffin/nodes/rendering/3d/pointlightnode3d.h"
#include "puffin/nodes/rendering/3d/spotlightnode3d.h"
#include "puffin/nodes/rendering/3d/directionallightnode3d.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"

namespace puffin::ui
{
	bool DrawTransformNode2DUI(ImGuiTreeNodeFlags flags, TransformNode2D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Transform 2D", flags))
		{
			auto& transform = node->Transform();

			{
#ifdef PFN_DOUBLE_PRECISION

				if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &transform.position, 2, 0.1f))
				{
					nodeChanged |= true;
				}

#else

				if (ImGui::DragFloat2("Position", reinterpret_cast<float*>(&transform.position), 0.1f))
				{
					nodeChanged |= true;
				}

#endif
			}

			{
				if (ImGui::DragFloat("Rotation", &transform.rotation, 0.1f, -180.0f, 180.0f))
				{
					nodeChanged |= true;
				}
			}

			{
				if (ImGui::DragFloat2("Scale", reinterpret_cast<float*>(&transform.scale), 0.1f))
				{
					nodeChanged |= true;
				}
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawTransformNode3DUI(ImGuiTreeNodeFlags flags, TransformNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Transform 3D", flags))
		{
			auto& transform = node->Transform();

			{
#ifdef PFN_USE_DOUBLE_PRECISION

				if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &transform.position, 3, 0.1f))
				{
					node->transform_3d().position = position;

					nodeChanged |= true;
				}

#else

				if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&transform.position), 0.1f))
				{
					nodeChanged |= true;
				}

#endif
			}

			{
				auto eulerAngles = transform.orientationEulerAngles;

				if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&eulerAngles), 0.2f, 0, 0, "%.3f"))
				{
					UpdateTransformOrientation(transform, eulerAngles);

					nodeChanged |= true;
				}
			}

			{
				if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&transform.scale), 0.1f))
				{
					nodeChanged |= true;
				}
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawStaticMeshNode3DUI(ImGuiTreeNodeFlags flags, rendering::StaticMeshNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Static Mesh 3D", flags))
		{
			ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

			ImGui::Text("Model UUID: %lu", node->GetMeshID());
			ImGui::Text("Material UUID: %lu", node->GetMaterialID());

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::LightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Light", flags))
		{
			if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&node->Color())))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragFloat("Ambient Intensity", &node->AmbientIntensity()))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragFloat("Specular Intensity", &node->SpecularIntensity()))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragInt("Specular Exponent", &node->SpecularExponent()))
			{
				nodeChanged |= true;
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawPointLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::PointLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Point Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawSpotLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::SpotLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Spot Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawDirLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::DirectionalLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Directional Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}
}