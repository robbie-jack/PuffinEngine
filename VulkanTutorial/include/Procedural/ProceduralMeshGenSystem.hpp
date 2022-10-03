#pragma once

#include "ECS/ECS.h"

namespace Puffin::Procedural
{
	class ProceduralMeshGenSystem : public ECS::System
	{
	public:

		ProceduralMeshGenSystem() = default;
		~ProceduralMeshGenSystem() override = default;

		void Init() override {}
		void PreStart() override {}
		void Start() override {}
		void Update() override;
		void Stop() override {}
		void Cleanup() override {}
	};
}
