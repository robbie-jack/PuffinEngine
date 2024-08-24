#pragma once

#include "puffin/nodes/rendering/3d/lightnode3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		class PointLightNode3D : public LightNode3D
		{
		public:

			explicit PointLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
			~PointLightNode3D() override = default;

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

			[[nodiscard]] const float& GetConstantAttenuation() const;
			[[nodiscard]] float& ConstantAttenuation();
			void SetConstantAttenuation(const float& constantAttenuation) const;

			[[nodiscard]] const float& GetLinearAttenuation() const;
			[[nodiscard]] float& LinearAttenuation();
			void SetLinearAttenuation(const float& linearAttenuation) const;

			[[nodiscard]] const float& GetQuadraticAttenuation() const;
			[[nodiscard]] float& QuadraticAttenuation();
			void SetQuadraticAttenuation(const float& quadraticAttenuation) const;

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::PointLightNode3D>()
	{
		using namespace rendering;

		entt::meta<PointLightNode3D>()
			.type(entt::hs("PointLightNode3D"))
			.base<LightNode3D>();
	}
}