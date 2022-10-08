#pragma once

#include "ECS/ECS.h"

#include "Components/Procedural/ProceduralMeshComponent.hpp"

namespace Puffin::Rendering::Procedural
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
			std::vector<Vertex_PNTV_32>& vertices, std::vector<uint32_t>& indices);

	};
}
