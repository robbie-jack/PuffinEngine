#include "puffin/nodes/rendering/3d/directionallightnode3d.h"

#include "puffin/components/rendering/3d/directionallightcomponent3d.h"

namespace puffin::rendering
{
	DirectionalLightNode3D::DirectionalLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
	}

	DirectionalLightNode3D::~DirectionalLightNode3D()
	{
	}

	const Vector3f& DirectionalLightNode3D::GetColor() const
	{
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

	void DirectionalLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& DirectionalLightNode3D::GetSpecularIntensity() const
	{
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

	void DirectionalLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<DirectionalLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}
}
