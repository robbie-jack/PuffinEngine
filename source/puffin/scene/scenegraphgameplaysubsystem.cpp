#include "puffin/scene/scenegraphgameplaysubsystem.h"

#include "puffin/core/subsystemmanager.h"
#include "puffin/scene/scenegraphsubsystem.h"

namespace puffin::scene
{
	SceneGraphGameplaySubsystem::SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "SceneGraphGameplaySubsystem";
	}

	void SceneGraphGameplaySubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		subsystemManager->CreateAndInitializeSubsystem<SceneGraphSubsystem>();
	}

	void SceneGraphGameplaySubsystem::BeginPlay()
	{
		const auto sceneGraph = mEngine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node)
				node->BeginPlay();
		}
	}

	void SceneGraphGameplaySubsystem::EndPlay()
	{
		const auto sceneGraph = mEngine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node)
				node->EndPlay();
		}
	}

	core::SubsystemType SceneGraphGameplaySubsystem::GetType() const
	{
		return core::SubsystemType::Gameplay;
	}

	void SceneGraphGameplaySubsystem::Update(double deltaTime)
	{
		const auto sceneGraph = mEngine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node && node->ShouldUpdate())
				node->Update(mEngine->GetDeltaTime());
		}
	}

	bool SceneGraphGameplaySubsystem::ShouldUpdate()
	{
		return true;
	}

	void SceneGraphGameplaySubsystem::FixedUpdate(double fixedTime)
	{
		const auto sceneGraph = mEngine->GetSubsystem<SceneGraphSubsystem>();

		for (auto& id : sceneGraph->GetNodeIDs())
		{
			if (const auto node = sceneGraph->GetNode(id); node && node->ShouldFixedUpdate())
				node->FixedUpdate(mEngine->GetTimeStepFixed());
		}
	}

	bool SceneGraphGameplaySubsystem::ShouldFixedUpdate()
	{
		return true;
	}
}
