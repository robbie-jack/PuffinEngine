#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	class PointLightNode3D : public TransformNode3D
	{
	public:

		explicit PointLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~PointLightNode3D() override;

		[[nodiscard]] virtual const Vector3f& GetColor() const;
		virtual void SetColor(const Vector3f& color) const;

		[[nodiscard]] virtual const float& GetAmbientIntensity() const;
		virtual void SetAmbientIntensity(const float& ambientIntensity) const;

		[[nodiscard]] const float& GetSpecularIntensity() const;
		void SetSpecularIntensity(const float& specularIntensity) const;

		[[nodiscard]] const int& GetSpecularExponent() const;
		void SetSpecularExponent(const int& specularExponent) const;

		[[nodiscard]] const int& GetConstantAttenutation() const;
		void SetConstantAttenutation(const int& constantAttenuation) const;

		[[nodiscard]] const int& GetLinearAttenutation() const;
		void SetLinearAttenutation(const int& linearAttenuation) const;

		[[nodiscard]] const int& GetQuadraticAttenutation() const;
		void SetQuadraticAttenutation(const int& quadraticAttenuation) const;

	private:



	};
}