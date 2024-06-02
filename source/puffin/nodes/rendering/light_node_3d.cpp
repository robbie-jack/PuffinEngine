#include "puffin/nodes/rendering/light_node_3d.h"

#include "puffin/components/rendering/light_component.h"

namespace puffin::rendering
{
	LightNode3D::LightNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id) : TransformNode3D(engine, id)
	{
		m_name = "Light";

		add_component<LightComponent>();
	}

	void LightNode3D::begin_play()
	{
		TransformNode3D::begin_play();
	}

	void LightNode3D::update(const double delta_time)
	{
		TransformNode3D::update(delta_time);
	}

	void LightNode3D::update_fixed(const double delta_time)
	{
		TransformNode3D::update_fixed(delta_time);
	}

	void LightNode3D::end_play()
	{
		TransformNode3D::end_play();
	}

	const Vector3f& LightNode3D::color() const
	{
		return get_component<LightComponent>().color;
	}

	void LightNode3D::set_color(const Vector3f& color) const
	{
		m_registry->patch<LightComponent>(m_entity, [&color](auto& light) { light.color = color; });
	}

	const float& LightNode3D::ambient_intensity() const
	{
		return get_component<LightComponent>().ambient_intensity;
	}

	void LightNode3D::set_ambient_intensity(const float& ambient_intensity) const
	{
		m_registry->patch<LightComponent>(m_entity, [&ambient_intensity](auto& light) { light.ambient_intensity = ambient_intensity; });
	}

	const float& LightNode3D::specular_intensity() const
	{
		return get_component<LightComponent>().specular_intensity;
	}

	void LightNode3D::set_specular_intensity(const float& specular_intensity) const
	{
		m_registry->patch<LightComponent>(m_entity, [&specular_intensity](auto& light) { light.specular_intensity = specular_intensity; });
	}

	const int& LightNode3D::specular_exponent() const
	{
		return get_component<LightComponent>().specular_exponent;
	}

	void LightNode3D::set_specular_exponent(const int& specular_exponent) const
	{
		m_registry->patch<LightComponent>(m_entity, [&specular_exponent](auto& light) { light.specular_exponent = specular_exponent; });
	}

	const float& LightNode3D::constant_attenuation() const
	{
		return get_component<LightComponent>().constant_attenuation;
	}

	void LightNode3D::set_constant_attenuation(const float& constant_attenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&constant_attenuation](auto& light) { light.constant_attenuation = constant_attenuation; });
	}

	const float& LightNode3D::linear_attenuation() const
	{
		return get_component<LightComponent>().linear_attenuation;
	}

	void LightNode3D::set_linear_attenuation(const float& linear_attenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&linear_attenuation](auto& light) { light.linear_attenuation = linear_attenuation; });
	}

	const float& LightNode3D::quadratic_attenuation() const
	{
		return get_component<LightComponent>().quadratic_attenuation;
	}

	void LightNode3D::set_quadratic_attenuation(const float& quadratic_attenuation) const
	{
		m_registry->patch<LightComponent>(m_entity, [&quadratic_attenuation](auto& light) { light.quadratic_attenuation = quadratic_attenuation; });
	}

	const float& LightNode3D::inner_cutoff_angle() const
	{
		return get_component<LightComponent>().inner_cutoff_angle;
	}

	void LightNode3D::set_inner_cutoff_angle(const float& inner_cutoff_angle) const
	{
		m_registry->patch<LightComponent>(m_entity, [&inner_cutoff_angle](auto& light) { light.inner_cutoff_angle = inner_cutoff_angle; });
	}

	const float& LightNode3D::outer_cutoff_angle() const
	{
		return get_component<LightComponent>().outer_cutoff_angle;
	}

	void LightNode3D::set_outer_cutoff_angle(const float& outer_cutoff_angle) const
	{
		m_registry->patch<LightComponent>(m_entity, [&outer_cutoff_angle](auto& light) { light.outer_cutoff_angle = outer_cutoff_angle; });
	}

	const LightType& LightNode3D::light_type() const
	{
		return get_component<LightComponent>().type;
	}

	void LightNode3D::set_light_type(const LightType& light_type) const
	{
		m_registry->patch<LightComponent>(m_entity, [&light_type](auto& light) { light.type = light_type; });
	}
}
