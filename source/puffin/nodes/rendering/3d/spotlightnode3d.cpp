#include "puffin/nodes/rendering/3d/spotlightnode3d.h"

#include "puffin/components/rendering/3d/spotlightcomponent3d.h"

namespace puffin::rendering
{
	SpotLightNode3D::SpotLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) :
		LightNode3D(engine, id)
	{
		mName = "Spot Light";
	}

	void SpotLightNode3D::Initialize()
	{
		LightNode3D::Initialize();

		AddComponent<SpotLightComponent3D>();
	}

	void SpotLightNode3D::Deinitialize()
	{
		LightNode3D::Deinitialize();

		RemoveComponent<SpotLightComponent3D>();
	}

	const std::string& SpotLightNode3D::GetTypeString() const
	{
		return gSpotLightNode3DTypeString;
	}

	entt::id_type SpotLightNode3D::GetTypeID() const
	{
		return gSpotLightNode3DTypeID;
	}

	LightType SpotLightNode3D::GetLightType()
	{
		return LightType::Spot;
	}

	const Vector3f& SpotLightNode3D::GetColor() const
	{
		return GetComponent<SpotLightComponent3D>().color;
	}

	Vector3f& SpotLightNode3D::Color()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().color;
	}

	void SpotLightNode3D::SetColor(const Vector3f& color) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&color](auto& light) { light.color = color; });
	}

	const float& SpotLightNode3D::GetAmbientIntensity() const
	{
		return GetComponent<SpotLightComponent3D>().ambientIntensity;
	}

	float& SpotLightNode3D::AmbientIntensity()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().ambientIntensity;
	}

	void SpotLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& SpotLightNode3D::GetSpecularIntensity() const
	{
		return GetComponent<SpotLightComponent3D>().specularIntensity;
	}

	float& SpotLightNode3D::SpecularIntensity()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().specularIntensity;
	}

	void SpotLightNode3D::SetSpecularIntensity(const float& specularIntensity) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });
	}

	const int& SpotLightNode3D::GetSpecularExponent() const
	{
		return GetComponent<SpotLightComponent3D>().specularExponent;
	}

	int& SpotLightNode3D::SpecularExponent()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().specularExponent;
	}

	void SpotLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}

	const float& SpotLightNode3D::GetConstantAttenuation() const
	{
		return GetComponent<SpotLightComponent3D>().constantAttenuation;
	}

	float& SpotLightNode3D::ConstantAttenuation()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().constantAttenuation;
	}

	void SpotLightNode3D::SetConstantAttenuation(const float& constantAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&constantAttenuation](auto& light) { light.constantAttenuation = constantAttenuation; });
	}

	const float& SpotLightNode3D::GetLinearAttenuation() const
	{
		return GetComponent<SpotLightComponent3D>().linearAttenuation;
	}

	float& SpotLightNode3D::LinearAttenuation()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().linearAttenuation;
	}

	void SpotLightNode3D::SetLinearAttenuation(const float& linearAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });
	}

	const float& SpotLightNode3D::GetQuadraticAttenuation() const
	{
		return GetComponent<SpotLightComponent3D>().quadraticAttenuation;
	}

	float& SpotLightNode3D::QuadraticAttenuation()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().quadraticAttenuation;
	}

	void SpotLightNode3D::SetQuadraticAttenuation(const float& quadraticAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });
	}

	const float& SpotLightNode3D::GetInnerCutoffAngle() const
	{
		return GetComponent<SpotLightComponent3D>().innerCutoffAngle;
	}

	float& SpotLightNode3D::InnerCutoffAngle()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().innerCutoffAngle;
	}

	void SpotLightNode3D::SetInnerCutoffAngle(const float& innerCutoffAngle) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&innerCutoffAngle](auto& light) { light.innerCutoffAngle = innerCutoffAngle; });
	}

	const float& SpotLightNode3D::GetOuterCutoffAngle() const
	{
		return GetComponent<SpotLightComponent3D>().outerCutoffAngle;
	}

	float& SpotLightNode3D::OuterCutoffAngle()
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity);

		return GetComponent<SpotLightComponent3D>().outerCutoffAngle;
	}

	void SpotLightNode3D::SetOuterCutoffAngle(const float& outerCutoffAngle) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&outerCutoffAngle](auto& light) { light.outerCutoffAngle = outerCutoffAngle; });
	}
}
