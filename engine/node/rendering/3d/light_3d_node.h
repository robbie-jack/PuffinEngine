﻿#pragma once

#include "node/transform_3d_node.h"
#include "component/rendering/3d/light_component_3d.h"
#include "utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		const std::string gLightNode3DTypeString = "LightNode3D";
		const entt::id_type gLightNode3DTypeID = entt::hs(gLightNode3DTypeString.c_str());

		class LightNode3D : public TransformNode3D
		{
		public:

			void Initialize() override;
			void Deinitialize() override;

			void Serialize(nlohmann::json& json) const override;
			void Deserialize(const nlohmann::json& json) override;

			[[nodiscard]] std::string_view GetTypeString() const override;
			[[nodiscard]] entt::id_type GetTypeID() const override;

			virtual LightType GetLightType() = 0;

			[[nodiscard]] virtual const Vector3f& GetColor() const = 0;
			[[nodiscard]] virtual Vector3f& Color() = 0;
			virtual void SetColor(const Vector3f& color) const = 0;

			[[nodiscard]] virtual const float& GetAmbientIntensity() const = 0;
			[[nodiscard]] virtual float& AmbientIntensity() = 0;
			virtual void SetAmbientIntensity(const float& ambientIntensity) const = 0;

			[[nodiscard]] virtual const float& GetSpecularIntensity() const = 0;
			[[nodiscard]] virtual float& SpecularIntensity() = 0;
			virtual void SetSpecularIntensity(const float& specularIntensity) const = 0;

			[[nodiscard]] virtual const int& GetSpecularExponent() const = 0;
			[[nodiscard]] virtual int& SpecularExponent() = 0;
			virtual void SetSpecularExponent(const int& specularExponent) const = 0;

			[[nodiscard]] bool GetCastShadows() const;
			void SetCastShadows(bool castShadows);

		private:

			bool mCastShadows = false;

		};
	}

	template<>
	inline void reflection::RegisterType<rendering::LightNode3D>()
	{
		using namespace rendering;

		entt::meta<LightNode3D>()
			.type(gLightNode3DTypeID)
			.base<TransformNode3D>()
			.custom<NodeCustomData>(gLightNode3DTypeString);
	}
}