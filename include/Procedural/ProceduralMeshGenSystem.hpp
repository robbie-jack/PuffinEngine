#pragma once

#include "ECS/ECS.h"

#include "Types/Vector.h"
#include "Types/Vertex.hpp"

namespace Puffin::Procedural
{
	class ProceduralMeshGenSystem : public ECS::System
	{
	public:

		ProceduralMeshGenSystem();
		~ProceduralMeshGenSystem() override = default;

		void Init() override {}
		void PreStart() override;
		void Start() override {}
		void Update() override;
		void Stop() override {}
		void Cleanup() override {}

	private:

		// Generator list of vertices/indices for a flat plane
		static void GeneratePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads,
			std::vector<Rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices);

		static void GenerateTerrain(std::vector<Rendering::VertexPNTV32>& vertices, const int64_t& seed, const double& heightMultiplier, const double&
		                            startFrequency, const int& octaves, const double& frequencyMultiplier);

		static void GenerateIcoSphere(std::vector<Rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices, const int& subdivisions);

	};
}
