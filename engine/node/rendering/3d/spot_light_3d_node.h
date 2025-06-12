#pragma once

#include "node/rendering/3d/light_3d_node.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gSpotLightNode3DTypeString = "SpotLightNode3D";
		const entt::id_type gSpotLightNode3DTypeID = entt::hs(gSpotLightNode3DTypeString.c_str());

		class SpotLightNode3D : public LightNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

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
			.type(gSpotLightNode3DTypeID)
			.base<LightNode3D>()
			.custom<NodeCustomData>(gSpotLightNode3DTypeString);
	}
}