#include "node/rendering/3d/directional_light_3d_node.h"

#include "component/rendering/3d/directional_light_component_3d.h"

namespace puffin::rendering
{
	void DirectionalLightNode3D::Initialize()
	{
		LightNode3D::Initialize();

		AddComponent<DirectionalLightComponent3D>();
	}

	void DirectionalLightNode3D::Deinitialize()
	{
		LightNode3D::Deinitialize();

		RemoveComponent<DirectionalLightComponent3D>();
	}

	std::string_view DirectionalLightNode3D::GetTypeString() const
	{
		return gDirectionalLightNode3DTypeString;
	}

	entt::id_type DirectionalLightNode3D::GetTypeID() const
	{
		return gDirectionalLightNode3DTypeID;
	}

	LightType DirectionalLightNode3D::GetLightType()
	{
		return LightType::Directional;
	}

	const Vector3f& DirectionalLightNode3D::GetColor() const
	{
		return GetComponent<DirectionalLightComponent3D>().color;
	}

	Vector3f& DirectionalLightNode3D::Color()
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity);

		return GetComponent<DirectionalLightComponent3D>().color;
	}

	void DirectionalLightNode3D::SetColor(const Vector3f& color) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&color](auto& light) { light.color = color; });
	}

	const float& DirectionalLightNode3D::GetAmbientIntensity() const
	{
		return GetComponent<DirectionalLightComponent3D>().ambientIntensity;
	}

	float& DirectionalLightNode3D::AmbientIntensity()
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity);

		return GetComponent<DirectionalLightComponent3D>().ambientIntensity;
	}

	void DirectionalLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& DirectionalLightNode3D::GetSpecularIntensity() const
	{
		return GetComponent<DirectionalLightComponent3D>().specularIntensity;
	}

	float& DirectionalLightNode3D::SpecularIntensity()
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity);

		return GetComponent<DirectionalLightComponent3D>().specularIntensity;
	}

	void DirectionalLightNode3D::SetSpecularIntensity(const float& specularIntensity) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });
	}

	const int& DirectionalLightNode3D::GetSpecularExponent() const
	{
		return GetComponent<DirectionalLightComponent3D>().specularExponent;
	}

	int& DirectionalLightNode3D::SpecularExponent()
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity);

		return GetComponent<DirectionalLightComponent3D>().specularExponent;
	}

	void DirectionalLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}
}
