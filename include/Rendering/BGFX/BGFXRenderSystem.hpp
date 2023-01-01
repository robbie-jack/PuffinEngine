#pragma once

#include "ECS/System.hpp"

namespace Puffin::Rendering
{
	class BGFXRenderSystem : public ECS::System
	{
	public:

		BGFXRenderSystem() = default;
		~BGFXRenderSystem() override;

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override {}
		void Stop() override {}
		void Cleanup() override {}

	};
}