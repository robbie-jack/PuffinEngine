﻿#pragma once

#include "puffin/nodes/rendering/3d/lightnode3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		class DirectionalLightNode3D : public LightNode3D
		{
		public:

			explicit DirectionalLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
			~DirectionalLightNode3D() override = default;

			void Initialize() override;
			void Deinitialize() override;

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

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::DirectionalLightNode3D>()
	{
		using namespace rendering;

		entt::meta<DirectionalLightNode3D>()
			.type(entt::hs("DirectionalLightNode3D"))
			.base<LightNode3D>();
	}
}