#include "puffin/nodes/rendering/3d/pointlightnode3d.h"

#include "puffin/components/rendering/3d/pointlightcomponent3d.h"

namespace puffin::rendering
{
	void PointLightNode3D::Initialize()
	{
		LightNode3D::Initialize();

		AddComponent<PointLightComponent3D>();
	}

	void PointLightNode3D::Deinitialize()
	{
		LightNode3D::Deinitialize();

		RemoveComponent<PointLightComponent3D>();
	}

	const std::string& PointLightNode3D::GetTypeString() const
	{
		return gPointLightNode3DTypeString;
	}

	entt::id_type PointLightNode3D::GetTypeID() const
	{
		return gPointLightNode3DTypeID;
	}

	LightType PointLightNode3D::GetLightType()
	{
		return LightType::Point;
	}

	const Vector3f& PointLightNode3D::GetColor() const
	{
		return GetComponent<PointLightComponent3D>().color;
	}

	Vector3f& PointLightNode3D::Color()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().color;
	}

	void PointLightNode3D::SetColor(const Vector3f& color) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&color](auto& light) { light.color = color; });
	}

	const float& PointLightNode3D::GetAmbientIntensity() const
	{
		return GetComponent<PointLightComponent3D>().ambientIntensity;
	}

	float& PointLightNode3D::AmbientIntensity()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().ambientIntensity;
	}

	void PointLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& PointLightNode3D::GetSpecularIntensity() const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().specularIntensity;
	}

	float& PointLightNode3D::SpecularIntensity()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().specularIntensity;
	}

	void PointLightNode3D::SetSpecularIntensity(const float& specularIntensity) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });
	}

	const int& PointLightNode3D::GetSpecularExponent() const
	{
		return GetComponent<PointLightComponent3D>().specularExponent;
	}

	int& PointLightNode3D::SpecularExponent()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().specularExponent;
	}

	void PointLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}

	const float& PointLightNode3D::GetConstantAttenuation() const
	{
		return GetComponent<PointLightComponent3D>().constantAttenuation;
	}

	float& PointLightNode3D::ConstantAttenuation()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().constantAttenuation;
	}

	void PointLightNode3D::SetConstantAttenuation(const float& constantAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&constantAttenuation](auto& light) { light.constantAttenuation = constantAttenuation; });
	}

	const float& PointLightNode3D::GetLinearAttenuation() const
	{
		return GetComponent<PointLightComponent3D>().linearAttenuation;
	}

	float& PointLightNode3D::LinearAttenuation()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().linearAttenuation;
	}

	void PointLightNode3D::SetLinearAttenuation(const float& linearAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });
	}

	const float& PointLightNode3D::GetQuadraticAttenuation() const
	{
		return GetComponent<PointLightComponent3D>().quadraticAttenuation;
	}

	float& PointLightNode3D::QuadraticAttenuation()
	{
		mRegistry->patch<PointLightComponent3D>(mEntity);

		return GetComponent<PointLightComponent3D>().quadraticAttenuation;
	}

	void PointLightNode3D::SetQuadraticAttenuation(const float& quadraticAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });
	}
}
