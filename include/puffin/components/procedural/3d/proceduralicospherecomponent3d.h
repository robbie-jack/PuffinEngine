#pragma once

#include <cstdint>
#include <vector>

#include "nlohmann/json.hpp"

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin
{
	namespace procedural
	{
		namespace icosahedron
		{
			constexpr float gX = .525731112119133606f;
			constexpr float gZ = .850650808352039932f;
			constexpr float gN = 0.f;

			static void VertexPositions(std::vector<Vector3f>& vertexPositions)
			{
				vertexPositions.clear();

				vertexPositions =
				{
					{-gX,gN,gZ}, {gX,gN,gZ}, {-gX,gN,-gZ}, {gX,gN,-gZ},
					{gN,gZ,gX}, {gN,gZ,-gX}, {gN,-gZ,gX}, {gN,-gZ,-gX},
					{gZ,gX,gN}, {-gZ,gX, gN}, {gZ,-gX,gN}, {-gZ,-gX, gN}
				};
			}

			static void Indices(std::vector<uint32_t>& indices)
			{
				indices.clear();

				indices =
				{
					0,4,1,0,9,4,9,5,4,4,5,8,4,8,1,
					8,10,1,8,3,10,5,3,8,5,2,3,2,7,3,
					7,10,3,7,6,10,7,11,6,11,0,6,0,1,6,
					6,1,10,9,0,11,9,11,2,9,2,5,7,2,11
				};
			}
		};

		struct ProceduralIcoSphereComponent3D
		{
			ProceduralIcoSphereComponent3D() = default;

			double radius = 0.5;
			int subdivisions = 10;

			NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProceduralIcoSphereComponent3D, radius, subdivisions)
		};
	}

	template<>
	inline void reflection::RegisterType<procedural::ProceduralIcoSphereComponent3D>()
	{
		using namespace procedural;

		entt::meta<ProceduralIcoSphereComponent3D>()
			.type(entt::hs("ProceduralIcoSphereComponent3D"))
			.data<&ProceduralIcoSphereComponent3D::radius>(entt::hs("radius"))
			.data<&ProceduralIcoSphereComponent3D::subdivisions>(entt::hs("subdivisions"));
	}

	namespace serialization
	{
		template<>
		inline void Serialize<procedural::ProceduralIcoSphereComponent3D>(const procedural::ProceduralIcoSphereComponent3D& data, Archive& archive)
		{
			archive.Set("radius", data.radius);
			archive.Set("subdivisions", data.subdivisions);
		}

		template<>
		inline void Deserialize<procedural::ProceduralIcoSphereComponent3D>(const Archive& archive, procedural::ProceduralIcoSphereComponent3D& data)
		{
			archive.Get("radius", data.radius);
			archive.Get("subdivisions", data.subdivisions);
		}

		template<>
		inline nlohmann::json Serialize<procedural::ProceduralIcoSphereComponent3D>(const procedural::ProceduralIcoSphereComponent3D& data)
		{
			nlohmann::json json;

			json["radius"] = data.radius;
			json["subdivisions"] = data.subdivisions;

			return json;
		}

		template<>
		inline procedural::ProceduralIcoSphereComponent3D Deserialize<procedural::ProceduralIcoSphereComponent3D>(const nlohmann::json& json)
		{
			procedural::ProceduralIcoSphereComponent3D data;

			data.radius = json["radius"];
			data.subdivisions = json["subdivisions"];
			
			return data;
		}
	}
}
