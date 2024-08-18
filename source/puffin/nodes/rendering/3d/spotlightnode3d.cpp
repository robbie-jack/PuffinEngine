#include "puffin/nodes/rendering/3d/spotlightnode3d.h"

#include "puffin/components/rendering/3d/spotlightcomponent3d.h"

namespace puffin::rendering
{
	SpotLightNode3D::SpotLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
		mName = "Spot Light";

		AddComponent<SpotLightComponent3D>();
	}

	SpotLightNode3D::~SpotLightNode3D()
	{
	}

	const Vector3f& SpotLightNode3D::GetColor() const
	{
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

	void SpotLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& SpotLightNode3D::GetSpecularIntensity() const
	{
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

	void SpotLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}

	const int& SpotLightNode3D::GetConstantAttenutation() const
	{
		return GetComponent<SpotLightComponent3D>().constantAttenuation;
	}

	void SpotLightNode3D::SetConstantAttenutation(const int& constantAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&constantAttenuation](auto& light) { light.constantAttenuation = constantAttenuation; });
	}

	const int& SpotLightNode3D::GetLinearAttenutation() const
	{
		return GetComponent<SpotLightComponent3D>().linearAttenuation;
	}

	void SpotLightNode3D::SetLinearAttenutation(const int& linearAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });
	}

	const int& SpotLightNode3D::GetQuadraticAttenutation() const
	{
		return GetComponent<SpotLightComponent3D>().quadraticAttenuation;
	}

	void SpotLightNode3D::SetQuadraticAttenutation(const int& quadraticAttenuation) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });
	}

	const int& SpotLightNode3D::GetInnerCutoffAngle() const
	{
		return GetComponent<SpotLightComponent3D>().innerCutoffAngle;
	}

	void SpotLightNode3D::SetInnerCutoffAngle(const int& innerCutoffAngle) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&innerCutoffAngle](auto& light) { light.innerCutoffAngle = innerCutoffAngle; });
	}

	const int& SpotLightNode3D::GetOuterCutoffAngle() const
	{
		return GetComponent<SpotLightComponent3D>().outerCutoffAngle;
	}

	void SpotLightNode3D::SetOuterCutoffAngle(const int& outerCutoffAngle) const
	{
		mRegistry->patch<SpotLightComponent3D>(mEntity, [&outerCutoffAngle](auto& light) { light.outerCutoffAngle = outerCutoffAngle; });
	}
}
