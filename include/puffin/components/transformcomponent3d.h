#pragma once

#include "nlohmann/json.hpp"

#include "puffin/types/eulerangles.h"
#include "puffin/types/vector3.h"
#include "puffin/types/quat.h"

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
//#ifdef PFN_DOUBLE_PRECISION
//	constexpr bool gUseDoublePrecision = true;
//#else
//	constexpr bool gUseDoublePrecision = false;
//#endif

	struct TransformComponent3D
	{
		TransformComponent3D() = default;

#ifdef PFN_DOUBLE_PRECISION
		TransformComponent3D(const Vector3d& position) : position(position) {}

		TransformComponent3D(const Vector3d& position, const maths::Quat& orientation, const maths::EulerAngles& euler, const Vector3f& scale) :
			position(position), orientation(orientation), orientation_euler_angles(euler), scale(scale) {}
#else
		explicit TransformComponent3D(const Vector3f& position) : position(position) {}

		TransformComponent3D(const Vector3f& position, const maths::Quat& orientation, const maths::EulerAngles& euler, const Vector3f& scale) :
			position(position), orientationQuat(orientation), orientationEulerAngles(euler), scale(scale) {}
#endif

		TransformComponent3D(const TransformComponent3D& t) = default;

		~TransformComponent3D() = default;

		TransformComponent3D& operator=(const TransformComponent3D& rhs) = default;

#ifdef PFN_DOUBLE_PRECISION
		Vector3d position = Vector3d(0.0);
#else
		Vector3f position = Vector3f(0.0f);
#endif

		maths::Quat orientationQuat = glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f }); // Orientation of transform, expressed as quaternion in radians
		maths::EulerAngles orientationEulerAngles = { 0.0, 0.0, 0.0 }; // Orientation of transform, expressed as euler angles in degrees

		Vector3f scale = Vector3f(1.0f);

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformComponent3D, position, orientationQuat, orientationEulerAngles, scale)
	};

	template<>
	inline void reflection::RegisterType<TransformComponent3D>()
	{
		entt::meta<TransformComponent3D>()
			.type(entt::hs("TransformComponent3D"))
			.data<&TransformComponent3D::position>(entt::hs("position"))
			.data<&TransformComponent3D::orientationQuat>(entt::hs("orientationQuat"))
			.data<&TransformComponent3D::orientationEulerAngles>(entt::hs("orientationEulerAngles"))
			.data<&TransformComponent3D::scale>(entt::hs("scale"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<TransformComponent3D>(const TransformComponent3D& data, Archive& archive)
		{
			archive.Set("position", data.position);
			archive.Set("orientationQuat", data.orientationQuat);
			archive.Set("orientationEulerAngles", data.orientationEulerAngles);
			archive.Set("scale", data.scale);
		}

		template<>
		inline void Deserialize<TransformComponent3D>(const Archive& archive, TransformComponent3D& data)
		{
			archive.Get("position", data.position);
			archive.Get("orientationQuat", data.orientationQuat);
			archive.Get("orientationEulerAngles", data.orientationEulerAngles);
			archive.Get("scale", data.scale);
		}

		template<>
		inline nlohmann::json Serialize<TransformComponent3D>(const TransformComponent3D& data)
		{
			nlohmann::json json;
			json["position"] = Serialize(data.position);
			json["orientationQuat"] = Serialize(data.orientationQuat);
			json["orientationEulerAngles"] = Serialize(data.orientationEulerAngles);
			json["scale"] = Serialize(data.scale);
			return json;
		}

		template<>
		inline TransformComponent3D Deserialize<TransformComponent3D>(const nlohmann::json& json)
		{
			TransformComponent3D data;
			data.position = json["position"];
			data.orientationQuat = json["orientationQuat"];
			data.orientationEulerAngles = json["orientationEulerAngles"];
			data.scale = json["scale"];
			return data;
		}
	}

	// Update transform orientation with a new euler angles and recalculates quaternion
	inline void UpdateTransformOrientation(TransformComponent3D& transform, const maths::EulerAngles& eulerAnglesNew)
	{
		transform.orientationEulerAngles = eulerAnglesNew;
		transform.orientationQuat = maths::EulerToQuat({ -transform.orientationEulerAngles }) * glm::angleAxis(0.0f, glm::vec3{ 0.0f, 0.0f, -1.0f });
	}
}
