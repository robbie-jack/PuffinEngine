#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	/*enum class LightType;

	class LightNode3D : public TransformNode3D
	{
	public:

		explicit LightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);

		void begin_play() override;
		void update(double deltaTime) override;
		void update_fixed(double deltaTime) override;
		void end_play() override;

		[[nodiscard]] virtual const Vector3f& GetColor() const;
		virtual void SetColor(const Vector3f& color) const;

		[[nodiscard]] virtual const float& GetAmbientIntensity() const;
		virtual void SetAmbientIntensity(const float& ambientIntensity) const;

		[[nodiscard]] const float& specular_intensity() const;
		void set_specular_intensity(const float& specularIntensity) const;

		[[nodiscard]] const int& specular_exponent() const;
		void set_specular_exponent(const int& specularExponent) const;

		[[nodiscard]] const float& constant_attenuation() const;
		void set_constant_attenuation(const float& constantAttenuation) const;

		[[nodiscard]] const float& linear_attenuation() const;
		void set_linear_attenuation(const float& linearAttenuation) const;

		[[nodiscard]] const float& quadratic_attenuation() const;
		void set_quadratic_attenuation(const float& quadraticAttenuation) const;

		[[nodiscard]] virtual const LightType& GetLightType() const = 0;

	private:

	};*/
}