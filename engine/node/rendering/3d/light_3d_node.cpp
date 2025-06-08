#include "node/rendering/3d/light_3d_node.h"

#include "component/rendering/3d/shadow_caster_component_3d.h"

namespace puffin::rendering
{
	void LightNode3D::Initialize()
	{
		TransformNode3D::Initialize();

		if (mCastShadows)
		{
			AddComponent<ShadowCasterComponent3D>();
		}
	}

	void LightNode3D::Deinitialize()
	{
		TransformNode3D::Deinitialize();

		if (mCastShadows)
		{
			RemoveComponent<ShadowCasterComponent3D>();
		}
	}

	void LightNode3D::Serialize(nlohmann::json& json) const
	{
		TransformNode3D::Serialize(json);

		json["castShadows"] = mCastShadows;
	}

	void LightNode3D::Deserialize(const nlohmann::json& json)
	{
		TransformNode3D::Deserialize(json);

		mCastShadows = json["castShadows"];
	}

	const std::string& LightNode3D::GetTypeString() const
	{
		return gLightNode3DTypeString;
	}

	entt::id_type LightNode3D::GetTypeID() const
	{
		return gLightNode3DTypeID;
	}

	bool LightNode3D::GetCastShadows() const
	{
		return mCastShadows;
	}

	void LightNode3D::SetCastShadows(bool castShadows)
	{
		if (mCastShadows == castShadows)
			return;

		if (castShadows == true)
		{
			if (!HasComponent<ShadowCasterComponent3D>())
			{
				AddComponent<ShadowCasterComponent3D>();
			}
		}
		else
		{
			if (HasComponent<ShadowCasterComponent3D>())
			{
				RemoveComponent<ShadowCasterComponent3D>();
			}
		}

		mCastShadows = castShadows;
	}
}
