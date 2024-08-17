#include "puffin/nodes/rendering/lightnode3d.h"

//#include "puffin/components/rendering/lightcomponent3d.h"

namespace puffin::rendering
{
	/*LightNode3D::LightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id) : TransformNode3D(engine, id)
	{
		m_name = "Light";

		add_component<LightComponent>();
	}

	void LightNode3D::begin_play()
	{
		TransformNode3D::begin_play();
	}

	void LightNode3D::update(const double deltaTime)
	{
		TransformNode3D::update(deltaTime);
	}

	void LightNode3D::update_fixed(const double deltaTime)
	{
		TransformNode3D::update_fixed(deltaTime);
	}

	void LightNode3D::end_play()
	{
		TransformNode3D::end_play();
	}

	const Vector3f& LightNode3D::GetColor() const
	{
		return get_component<LightComponent>().color;
	}

	void LightNode3D::SetColor(const Vector3f& color) const
	{
		m_registry->patch<LightComponent>(m_entity, [&color](auto& light) { light.color = color; });
	}

	const float& LightNode3D::GetAmbientIntensity() const
	{
		return get_component<LightComponent>().ambientIntensity;
	}

	void LightNode3D::SetAmbientIntensity(const float& ambientIntensity) const
	{
		m_registry->patch<LightComponent>(m_entity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });
	}

	const float& LightNode3D::specular_intensity() const
	{
		return get_component<LightComponent>().specularIntensity;
	}

	void LightNode3D::set_specular_intensity(const float& specularIntensity) const
	{
		m_registry->patch<LightComponent>(m_entity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });
	}

	const int& LightNode3D::specular_exponent() const
	{
		return get_component<LightComponent>().specularExponent;
	}

	void LightNode3D::set_specular_exponent(const int& specularExponent) const
	{
		m_registry->patch<LightComponent>(m_entity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });
	}

	const float& LightNode3D::constant_attenuation() const
	{
		return get_component<LightComponent>().constantAttenuation;
	}

	void LightNode3D::set_constant_attenuation(const float& constantAttenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&constantAttenuation](auto& light) { light.constantAttenuation = constantAttenuation; });
	}

	const float& LightNode3D::linear_attenuation() const
	{
		return get_component<LightComponent>().linearAttenuation;
	}

	void LightNode3D::set_linear_attenuation(const float& linearAttenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });
	}

	const float& LightNode3D::quadratic_attenuation() const
	{
		return get_component<LightComponent>().quadraticAttenuation;
	}

	void LightNode3D::set_quadratic_attenuation(const float& quadraticAttenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });
	}

	const float& LightNode3D::inner_cutoff_angle() const
	{
		return get_component<LightComponent>().innerCutoffAngle;
	}

	void LightNode3D::set_inner_cutoff_angle(const float& innerCutoffAngle) const
	{
		m_registry->patch<LightComponent>(m_entity, [&innerCutoffAngle](auto& light) { light.innerCutoffAngle = innerCutoffAngle; });
	}

	const float& LightNode3D::outer_cutoff_angle() const
	{
		return get_component<LightComponent>().outerCutoffAngle;
	}

	void LightNode3D::set_outer_cutoff_angle(const float& outerCutoffAngle) const
	{
		m_registry->patch<LightComponent>(m_entity, [&outerCutoffAngle](auto& light) { light.outerCutoffAngle = outerCutoffAngle; });
	}

	const LightType& LightNode3D::GetLightType() const
	{
		return get_component<LightComponent>().type;
	}

	void LightNode3D::set_light_type(const LightType& lightType) const
	{
		m_registry->patch<LightComponent>(m_entity, [&lightType](auto& light) { light.type = lightType; });
	}*/
}
