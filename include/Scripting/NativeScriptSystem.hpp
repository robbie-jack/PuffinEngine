#pragma once

#include "ECS/ECS.h"

namespace Puffin::Scripting
{
	class NativeScriptSystem : public ECS::System
	{
	public:

		NativeScriptSystem()
		{
			m_systemInfo.name = "NativeScriptSystem";
			m_systemInfo.updateOrder = Core::UpdateOrder::Update;
		}

		~NativeScriptSystem() override = default;

		void Init() override {}
		void PreStart() override {}
		void Start() override;
		void Update() override;
		void Stop() override;
		void Cleanup() override {}

	};
}