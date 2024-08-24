﻿#pragma once

#include "puffin/nodes/transformnode3d.h"
#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/utility/reflection.h"

namespace puffin
{
	namespace rendering
	{
		class LightNode3D : public TransformNode3D
		{
		public:

			explicit LightNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
			~LightNode3D() override = default;

			void Initialize() override;
			void Deinitialize() override;

			void Serialize(serialization::Archive& archive) const override;
			void Deserialize(const serialization::Archive& archive) override;

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
			.type(entt::hs("LightNode3D"))
			.base<TransformNode3D>();
	}
}