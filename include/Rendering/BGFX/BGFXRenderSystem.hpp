#pragma once

#include "ECS/System.hpp"

namespace Puffin::Rendering
{
	class BGFXRenderSystem : public ECS::System
	{
	public:

		BGFXRenderSystem()
		{
			m_systemInfo.name = "BGFXRenderSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Render;
		}

		~BGFXRenderSystem() override {}

		void Init() override;
		void PreStart() override {}
		void Start() override {}
		void Update() override {}
		void Stop() override {}
		void Cleanup() override {}

	};
}