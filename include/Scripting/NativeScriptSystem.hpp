#pragma once

#include "ECS/ECS.h"
#include "Engine/Engine.hpp"

namespace Puffin::Scripting
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
			m_engine->RegisterCallback(Core::ExecutionStage::Start, [&]() { Start(); }, "NativeScriptSystem: Start");
			m_engine->RegisterCallback(Core::ExecutionStage::Update, [&]() { Update(); }, "NativeScriptSystem: Update");
			m_engine->RegisterCallback(Core::ExecutionStage::Stop, [&]() { Stop(); }, "NativeScriptSystem: Stop");
		}

		void Start();
		void Update();
		void Stop();

	};
}