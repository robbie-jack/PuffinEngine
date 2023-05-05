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
			m_systemInfo.name = "ProceduralMeshGenSystem";
		}

		~ProceduralMeshGenSystem() override = default;

		void SetupCallbacks() override
		{
			m_engine->registerCallback(core::ExecutionStage::Setup, [&]() { Setup(); }, "ProcMeshGenSystem: Setup");
			m_engine->registerCallback(core::ExecutionStage::Update, [&]() { Update(); }, "ProcMeshGenSystem: Update", 200);
		}

		void Setup();
		void Update();

	private:

		// Generator list of vertices/indices for a flat plane
		static void GeneratePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads,
			std::vector<rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices);

		static void GenerateTerrain(std::vector<rendering::VertexPNTV32>& vertices, const int64_t& seed, const double& heightMultiplier, const double&
		                            startFrequency, const int& octaves, const double& frequencyMultiplier);

		static void GenerateIcoSphere(std::vector<rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices, const int& subdivisions);

	};
}
