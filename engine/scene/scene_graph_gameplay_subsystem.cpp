#include "scene/scene_graph_gameplay_subsystem.h"

#include "core/engine.h"
#include "subsystem/subsystem_manager.h"
#include "scene/scene_graph_subsystem.h"

namespace puffin::scene
{
	SceneGraphGameplaySubsystem::SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine) : GameplaySubsystem(engine)
	{
	}

	void SceneGraphGameplaySubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		GameplaySubsystem::Initialize(subsystemManager);

		subsystemManager->CreateAndInitializeSubsystem<SceneGraphSubsystem>();
	}

	void SceneGraphGameplaySubsystem::BeginPlay()
	{
		const auto sceneGraph = m_engine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node)
				node->BeginPlay();
		}
	}

	void SceneGraphGameplaySubsystem::EndPlay()
	{
		const auto sceneGraph = m_engine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node)
				node->EndPlay();
		}
	}

	void SceneGraphGameplaySubsystem::Update(double deltaTime)
	{
		const auto sceneGraph = m_engine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node && node->ShouldUpdate())
				node->Update(m_engine->GetDeltaTime());
		}
	}

	bool SceneGraphGameplaySubsystem::ShouldUpdate()
	{
		return true;
	}

	void SceneGraphGameplaySubsystem::FixedUpdate(double fixedTime)
	{
		const auto sceneGraph = m_engine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node && node->ShouldFixedUpdate())
				node->FixedUpdate(m_engine->GetTimeStepFixed());
		}
	}

	bool SceneGraphGameplaySubsystem::ShouldFixedUpdate()
	{
		return true;
	}

	std::string_view SceneGraphGameplaySubsystem::GetName() const
	{
		return reflection::GetTypeString<SceneGraphGameplaySubsystem>();
	}
}
