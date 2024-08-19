﻿#include "puffin/nodes/rendering/3d/directionallightnode3d.h"

#include "puffin/components/rendering/3d/directionallightcomponent3d.h"

namespace puffin::rendering
{
	DirectionalLightNode3D::DirectionalLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) :
		LightNode3D(engine, id)
	{
		mName = "Directional Light";
	}

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
