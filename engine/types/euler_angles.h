#pragma once

#include "types/quat.h"
#include "types/vector3.h"

#include "math_helpers.h"

namespace puffin
{
	namespace maths
	{
		struct EulerAngles
		{
			EulerAngles() : pitch(0.0f), yaw(0.0f), roll(0.0f) {}
			EulerAngles(const float& pitch, const float& yaw, const float& roll) : pitch(pitch), yaw(yaw), roll(roll) {}
			explicit EulerAngles(const glm::vec3& vec) : pitch(vec.x), yaw(vec.y), roll(vec.z) {}
			explicit EulerAngles(const Vector3f& vec) : pitch(vec.x), yaw(vec.y), roll(vec.z) {}

			EulerAngles operator+(const EulerAngles& other) const
			{
				EulerAngles euler(pitch, yaw, roll);
				euler.pitch += other.pitch;
				euler.yaw += other.yaw;
				euler.roll += other.roll;

				return euler;
			}

			EulerAngles& operator+=(const EulerAngles& other)
			{
				pitch += other.pitch;
				yaw += other.yaw;
				roll += other.roll;

				return *this;
			}

			EulerAngles operator-(const EulerAngles& other) const
			{
				EulerAngles euler(pitch, yaw, roll);
				euler.pitch -= other.pitch;
				euler.yaw -= other.yaw;
				euler.roll -= other.roll;

				return euler;
			}

			EulerAngles operator-() const
			{
				return { -pitch, -yaw, -roll };
			}

			float pitch, yaw, roll;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(EulerAngles, pitch, yaw, roll)
		};

		inline EulerAngles DegToRad(const EulerAngles& euler)
		{
			return { DegToRad(euler.pitch), DegToRad(euler.yaw), DegToRad(euler.roll) };
		}

		inline EulerAngles RadToDeg(const EulerAngles& euler)
		{
			return { RadToDeg(euler.pitch), RadToDeg(euler.yaw), RadToDeg(euler.roll) };
		}

		// Convert from euler in degrees to quaternion in radians
		inline maths::Quat EulerToQuat(const maths::EulerAngles& euler)
		{
			const auto eulerRad = maths::DegToRad(euler);

			return { glm::quat(glm::vec3(eulerRad.pitch, eulerRad.yaw, eulerRad.roll)) };
		}

		// Convert from euler in degrees to quaternion in radians
		inline maths::EulerAngles QuatToEuler(const maths::Quat& quat)
		{
			return maths::RadToDeg(maths::EulerAngles(glm::eulerAngles(glm::quat(quat))));
		}
	}

	template<>
	inline void reflection::RegisterType<maths::EulerAngles>()
	{
		using namespace maths;

		entt::meta<EulerAngles>()
			.type(entt::hs("EulerAngles"))
			.data<&EulerAngles::pitch>(entt::hs("pitch"))
			.data<&EulerAngles::yaw>(entt::hs("yaw"))
			.data<&EulerAngles::roll>(entt::hs("roll"));
	}

	namespace serialization
	{
		template<>
		inline nlohmann::json Serialize<maths::EulerAngles>(const maths::EulerAngles& data)
		{
			nlohmann::json json;
			json["pitch"] = data.pitch;
			json["yaw"] = data.yaw;
			json["roll"] = data.roll;
			return json;
		}

		template<>
		inline maths::EulerAngles Deserialize<maths::EulerAngles>(const nlohmann::json& json)
		{
			maths::EulerAngles data;
			data.pitch = json["pitch"];
			data.yaw = json["yaw"];
			data.roll = json["roll"];
			return data;
		}
	}
}