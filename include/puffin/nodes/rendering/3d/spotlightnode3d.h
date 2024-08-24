#pragma once

#include "puffin/nodes/rendering/3d/lightnode3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		class SpotLightNode3D : public LightNode3D
		{
		public:

			explicit SpotLightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
			~SpotLightNode3D() override = default;

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

			[[nodiscard]] const float& GetInnerCutoffAngle() const;
			[[nodiscard]] float& InnerCutoffAngle();
			void SetInnerCutoffAngle(const float& innerCutoffAngle) const;

			[[nodiscard]] const float& GetOuterCutoffAngle() const;
			[[nodiscard]] float& OuterCutoffAngle();
			void SetOuterCutoffAngle(const float& outerCutoffAngle) const;

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::SpotLightNode3D>()
	{
		using namespace rendering;

		entt::meta<SpotLightNode3D>()
			.type(entt::hs("SpotLightNode3D"))
			.base<LightNode3D>();
	}
}