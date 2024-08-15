#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	enum class LightType;

	class LightNode3D : public TransformNode3D
	{
	public:

		explicit LightNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);

		void begin_play() override;
		void update(const double delta_time) override;
		void update_fixed(const double delta_time) override;
		void end_play() override;

		[[nodiscard]] const Vector3f& color() const;
		void set_color(const Vector3f& color) const;

		[[nodiscard]] const float& ambient_intensity() const;
		void set_ambient_intensity(const float& ambient_intensity) const;

		[[nodiscard]] const float& specular_intensity() const;
		void set_specular_intensity(const float& specular_intensity) const;

		[[nodiscard]] const int& specular_exponent() const;
		void set_specular_exponent(const int& specular_exponent) const;

		[[nodiscard]] const float& constant_attenuation() const;
		void set_constant_attenuation(const float& constant_attenuation) const;

		[[nodiscard]] const float& linear_attenuation() const;
		void set_linear_attenuation(const float& linear_attenuation) const;

		[[nodiscard]] const float& quadratic_attenuation() const;
		void set_quadratic_attenuation(const float& quadratic_attenuation) const;

		[[nodiscard]] const float& inner_cutoff_angle() const;
		void set_inner_cutoff_angle(const float& inner_cutoff_angle) const;

		[[nodiscard]] const float& outer_cutoff_angle() const;
		void set_outer_cutoff_angle(const float& outer_cutoff_angle) const;

		[[nodiscard]] const LightType& light_type() const;
		void set_light_type(const LightType& light_type) const;

	private:

	};
}