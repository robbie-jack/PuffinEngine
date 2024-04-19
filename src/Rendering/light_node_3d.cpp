#include "Rendering/light_node_3d.h"
#include "Components/Rendering/LightComponent.h"

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

	void LightNode3D::physics_update(const double delta_time)
	{
		TransformNode3D::physics_update(delta_time);
	}

	void LightNode3D::end_play()
	{
		TransformNode3D::end_play();
	}

	Vector3f& LightNode3D::color()
	{
		return get_component<LightComponent>().color;
	}

	Vector3f& LightNode3D::direction()
	{
		return get_component<LightComponent>().direction;
	}

	float& LightNode3D::ambient_intensity()
	{
		return get_component<LightComponent>().ambientIntensity;
	}

	float& LightNode3D::specular_intensity()
	{
		return get_component<LightComponent>().specularIntensity;
	}

	int& LightNode3D::specular_exponent()
	{
		return get_component<LightComponent>().specularExponent;
	}

	float& LightNode3D::constant_attenuation()
	{
		return get_component<LightComponent>().constantAttenuation;
	}

	float& LightNode3D::linear_attenuation()
	{
		return get_component<LightComponent>().linearAttenuation;
	}

	float& LightNode3D::quadratic_attenuation()
	{
		return get_component<LightComponent>().quadraticAttenuation;
	}

	float& LightNode3D::inner_cutoff_angle()
	{
		return get_component<LightComponent>().innerCutoffAngle;
	}

	float& LightNode3D::outer_cutoff_angle()
	{
		return get_component<LightComponent>().outerCutoffAngle;
	}

	LightType& LightNode3D::light_type()
	{
		return get_component<LightComponent>().type;
	}
}
