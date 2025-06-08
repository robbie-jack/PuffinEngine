#pragma once

#include "node/rendering/3d/light_3d_node.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gPointLightNode3DTypeString = "PointLightNode3D";
		const entt::id_type gPointLightNode3DTypeID = entt::hs(gPointLightNode3DTypeString.c_str());

		class PointLightNode3D : public LightNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			[[nodiscard]] const std::string& GetTypeString() const override;
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

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::PointLightNode3D>()
	{
		using namespace rendering;

		entt::meta<PointLightNode3D>()
			.type(gPointLightNode3DTypeID)
			.base<LightNode3D>()
			.custom<NodeCustomData>(gPointLightNode3DTypeString);
	}
}