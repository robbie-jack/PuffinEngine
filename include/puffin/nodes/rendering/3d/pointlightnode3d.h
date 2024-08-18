#pragma once

#include "puffin/nodes/rendering/3d/lightnode3d.h"

namespace puffin::rendering
{
	class PointLightNode3D : public LightNode3D
	{
	public:

		explicit PointLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~PointLightNode3D() override;

		[[nodiscard]] LightType GetLightType() override;

		[[nodiscard]] const Vector3f& GetColor() const override;
		[[nodiscard]] Vector3f& Color() override;
		void SetColor(const Vector3f& color) const override;

		[[nodiscard]] const float& GetAmbientIntensity() const override;
		[[nodiscard]] float& AmbientIntensity() override;
		void SetAmbientIntensity(const float& ambientIntensity) const override;

		[[nodiscard]] const float& GetSpecularIntensity() const override;
		[[nodiscard]] float& SpecularIntensity() override;
		void SetSpecularIntensity(const float& specularIntensity) const override;

		[[nodiscard]] const int& GetSpecularExponent() const override;
		[[nodiscard]] int& SpecularExponent() override;
		void SetSpecularExponent(const int& specularExponent) const override;

		[[nodiscard]] const int& GetConstantAttenuation() const;
		void SetConstantAttenuation(const int& constantAttenuation) const;

		[[nodiscard]] const int& GetLinearAttenuation() const;
		void SetLinearAttenuation(const int& linearAttenuation) const;

		[[nodiscard]] const int& GetQuadraticAttenuation() const;
		void SetQuadraticAttenuation(const int& quadraticAttenuation) const;

	private:



	};
}