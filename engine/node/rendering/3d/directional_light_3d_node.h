#pragma once

#include "node/rendering/3d/light_3d_node.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gDirectionalLightNode3DTypeString = "DirectionalLightNode3D";
		const entt::id_type gDirectionalLightNode3DTypeID = entt::hs(gDirectionalLightNode3DTypeString.c_str());

		class DirectionalLightNode3D : public LightNode3D
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

		private:



		};
	}

	template<>
	inline void reflection::RegisterType<rendering::DirectionalLightNode3D>()
	{
		using namespace rendering;

		entt::meta<DirectionalLightNode3D>()
			.type(gDirectionalLightNode3DTypeID)
			.base<LightNode3D>()
			.custom<NodeCustomData>(gDirectionalLightNode3DTypeString);
	}
}