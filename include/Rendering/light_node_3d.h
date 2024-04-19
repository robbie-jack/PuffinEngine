#pragma once

#include "scene/transform_node_3d.h"

namespace puffin::rendering
{
	enum class LightType;

	class LightNode3D : public scene::TransformNode3D
	{
	public:

		explicit LightNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

		Vector3f& color();
		Vector3f& direction();

		float& ambient_intensity();
		float& specular_intensity();
		int& specular_exponent();

		float& constant_attenuation();
		float& linear_attenuation();
		float& quadratic_attenuation();

		float& inner_cutoff_angle();
		float& outer_cutoff_angle();

		LightType& light_type();

	private:

	};
}