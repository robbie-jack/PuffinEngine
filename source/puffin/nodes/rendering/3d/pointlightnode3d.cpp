#include "puffin/nodes/rendering/3d/pointlightnode3d.h"

#include "puffin/components/rendering/3d/pointlightcomponent3d.h"

namespace puffin::rendering
{
	PointLightNode3D::PointLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) :
		TransformNode3D(engine, id)
	{
	}

	PointLightNode3D::~PointLightNode3D()
	{
	}

	const Vector3f& PointLightNode3D::GetColor() const
	{
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

	void PointLightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& PointLightNode3D::GetSpecularIntensity() const
	{
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

	void PointLightNode3D::SetSpecularExponent(const int& specularExponent) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}

	const int& PointLightNode3D::GetConstantAttenutation() const
	{
		return GetComponent<PointLightComponent3D>().constantAttenuation;
	}

	void PointLightNode3D::SetConstantAttenutation(const int& constantAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&constantAttenuation](auto& light) { light.constantAttenuation = constantAttenuation; });
	}

	const int& PointLightNode3D::GetLinearAttenutation() const
	{
		return GetComponent<PointLightComponent3D>().linearAttenuation;
	}

	void PointLightNode3D::SetLinearAttenutation(const int& linearAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });
	}

	const int& PointLightNode3D::GetQuadraticAttenutation() const
	{
		return GetComponent<PointLightComponent3D>().quadraticAttenuation;
	}

	void PointLightNode3D::SetQuadraticAttenutation(const int& quadraticAttenuation) const
	{
		mRegistry->patch<PointLightComponent3D>(mEntity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });
	}
}
