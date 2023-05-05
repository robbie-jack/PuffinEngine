#pragma once

#include "ECS/ECS.h"
#include "Engine/Engine.hpp"

#include "Types/Vector.h"
#include "Types/Vertex.hpp"

namespace puffin::procedural
{
	class ProceduralMeshGenSystem : public ECS::System
	{
	public:

		ProceduralMeshGenSystem()
		{
			mSystemInfo.name = "ProceduralMeshGenSystem";
		}

		~ProceduralMeshGenSystem() override = default;

		void setupCallbacks() override
		{
			mEngine->registerCallback(core::ExecutionStage::Setup, [&]() { setup(); }, "ProcMeshGenSystem: Setup");
			mEngine->registerCallback(core::ExecutionStage::Update, [&]() { update(); }, "ProcMeshGenSystem: Update", 200);
		}

		void setup() const;
		void update() const;

	private:

		// Generator list of vertices/indices for a flat plane
		static void generatePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads,
			std::vector<rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices);

		static void generateTerrain(std::vector<rendering::VertexPNTV32>& vertices, const int64_t& seed, const double& heightMultiplier, const double&
		                            startFrequency, const int& octaves, const double& frequencyMultiplier);

		static void generateIcoSphere(std::vector<rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices, const int& subdivisions);

	};
}
