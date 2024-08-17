﻿#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::rendering
{
	class DirectionalLightNode3D : public TransformNode3D
	{
	public:

		explicit DirectionalLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~DirectionalLightNode3D() override;

		[[nodiscard]] virtual const Vector3f& GetColor() const;
		virtual void SetColor(const Vector3f& color) const;

		[[nodiscard]] virtual const float& GetAmbientIntensity() const;
		virtual void SetAmbientIntensity(const float& ambientIntensity) const;

		[[nodiscard]] const float& GetSpecularIntensity() const;
		void SetSpecularIntensity(const float& specularIntensity) const;

		[[nodiscard]] const int& GetSpecularExponent() const;
		void SetSpecularExponent(const int& specularExponent) const;

	private:



	};
}