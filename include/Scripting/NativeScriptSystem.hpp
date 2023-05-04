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
			m_engine->registerCallback(Core::ExecutionStage::start, [&]() { Start(); }, "NativeScriptSystem: Start");
			m_engine->registerCallback(Core::ExecutionStage::update, [&]() { Update(); }, "NativeScriptSystem: Update");
			m_engine->registerCallback(Core::ExecutionStage::stop, [&]() { Stop(); }, "NativeScriptSystem: Stop");
		}

		void Start();
		void Update();
		void Stop();

	};
}