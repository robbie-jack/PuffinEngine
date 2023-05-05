#pragma once

#include "ECS/ECS.h"
#include "Engine/Engine.hpp"

namespace puffin::scripting
{
	class NativeScriptSystem : public ECS::System
	{
	public:

		NativeScriptSystem()
		{
			m_systemInfo.name = "NativeScriptSystem";
		}

		~NativeScriptSystem() override = default;

		void SetupCallbacks() override
		{
			m_engine->registerCallback(core::ExecutionStage::Start, [&]() { Start(); }, "NativeScriptSystem: Start");
			m_engine->registerCallback(core::ExecutionStage::Update, [&]() { Update(); }, "NativeScriptSystem: Update");
			m_engine->registerCallback(core::ExecutionStage::Stop, [&]() { Stop(); }, "NativeScriptSystem: Stop");
		}

		void Start();
		void Update();
		void Stop();

	};
}