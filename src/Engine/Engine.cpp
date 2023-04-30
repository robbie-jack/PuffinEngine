#include "Engine/Engine.hpp"

#include "ECS/ECS.h"
#include "ECS/Entity.h"

#include "Rendering/BGFX/BGFXRenderSystem.hpp"
#include "Rendering/Vulkan/VKRenderSystem.hpp"
//#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Physics/Onager2D/Onager2DPhysicsSystem.h"
#include "Scripting/AngelScriptSystem.h"
#include "Scripting/NativeScriptSystem.hpp"
#include "Procedural/ProceduralMeshGenSystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Scripting/AngelScriptComponent.hpp"
#include "Components/Scripting/NativeScriptComponent.hpp"
#include "Components/Procedural/ProceduralMeshComponent.hpp"

#include "Types/ComponentFlags.h"

#include "Window/WindowSubsystem.hpp"
#include "Input/InputSubsystem.h"
#include "Engine/SignalSubsystem.hpp"
#include "Engine/EventSubsystem.hpp"

#include "SerializeScene.h"
#include "UI/Editor/UIManager.h"

#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Assets/SoundAsset.h"

#include "Assets/AssetImporters.hpp"
#include "Components/Physics/VelocityComponent.hpp"

#include "Engine/JobSystem.hpp"

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace Puffin::Core
{
	void Engine::Init()
	{
		// Subsystems

		// Insert with priority 0 so window subsystem is initialized first
		auto windowSubsystem = RegisterSubsystem<Window::WindowSubsystem>(0);

		auto eventSubsystem = RegisterSubsystem<Core::EventSubsystem>();
		auto signalSubsystem = RegisterSubsystem<Core::SignalSubsystem>();
		auto inputSubsystem = RegisterSubsystem<Input::InputSubsystem>();
		auto audioSubsystem = RegisterSubsystem<Audio::AudioSubsystem>();
		auto ecsWorld = RegisterSubsystem<ECS::World>();

		m_uiManager = std::make_shared<UI::UIManager>(shared_from_this());

		// Load Project File
		fs::path projectPath = fs::path("C:\\Projects\\PuffinProject\\Puffin.pproject");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		IO::LoadProject(projectPath, projectFile);

		// Load Default Scene (if set)
		fs::path defaultScenePath = projectDirPath.parent_path() / "content" / projectFile.defaultScenePath;
		m_sceneData = std::make_shared<IO::SceneData>(ecsWorld, defaultScenePath);

		// Register Components to ECS World and Scene Data Class
		RegisterComponent<TransformComponent>();

		RegisterComponent<Rendering::MeshComponent>();
		RegisterComponent<Rendering::LightComponent>();
		RegisterComponent<Rendering::ShadowCasterComponent>();
		RegisterComponent<Rendering::CameraComponent>();

		RegisterComponent<Physics::VelocityComponent>(false);

		RegisterComponent<Physics::RigidbodyComponent2D>();
		RegisterComponent<Physics::BoxComponent2D>();
		RegisterComponent<Physics::CircleComponent2D>();

		RegisterComponent<Scripting::AngelScriptComponent>();
		RegisterComponent<Scripting::NativeScriptComponent>();

		RegisterComponent<Rendering::ProceduralMeshComponent>();
		RegisterComponent<Procedural::PlaneComponent>();
		RegisterComponent<Procedural::TerrainComponent>();
		RegisterComponent<Procedural::IcoSphereComponent>();

		ecsWorld->AddComponentDependencies<Rendering::MeshComponent, TransformComponent>();
		ecsWorld->AddComponentDependencies<Rendering::LightComponent, TransformComponent>();
		ecsWorld->AddComponentDependencies<Rendering::ShadowCasterComponent, Rendering::LightComponent>();

		ecsWorld->AddComponentDependencies<Physics::RigidbodyComponent2D, TransformComponent>();
		ecsWorld->AddComponentDependencies<Physics::RigidbodyComponent2D, Physics::VelocityComponent>();

		ecsWorld->AddComponentDependencies<Procedural::PlaneComponent, Rendering::ProceduralMeshComponent>();
		ecsWorld->AddComponentDependencies<Procedural::TerrainComponent, Rendering::ProceduralMeshComponent>();
		ecsWorld->AddComponentDependencies<Procedural::IcoSphereComponent, Rendering::ProceduralMeshComponent>();

		// Register Entity Flags

		// Register Component Flags
		ecsWorld->RegisterComponentFlag<FlagDirty>(true);
		ecsWorld->RegisterComponentFlag<FlagDeleted>();

		// Systems
		//RegisterSystem<Rendering::BGFX::BGFXRenderSystem>();
		RegisterSystem<Rendering::VK::VKRenderSystem>();
		RegisterSystem<Physics::Onager2DPhysicsSystem>();
		//RegisterSystem<Physics::Box2DPhysicsSystem>();
		RegisterSystem<Scripting::AngelScriptSystem>();
		RegisterSystem<Scripting::NativeScriptSystem>();
		RegisterSystem<Procedural::ProceduralMeshGenSystem>();

		// Register Assets
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::StaticMeshAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::TextureAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::SoundAsset>();

		// Load Asset Cache
		Assets::AssetRegistry::Get()->ProjectName(projectFile.name);
		Assets::AssetRegistry::Get()->ProjectRoot(projectDirPath);

		// Load Project Settings
		IO::LoadSettings(projectDirPath.parent_path() / "Settings.json", settings);

		// Load/Initialize Assets
		//AddDefaultAssets();
		Assets::AssetRegistry::Get()->LoadAssetCache();
		//ReimportDefaultAssets();

		Core::JobSystem::Get()->Start();

		// Create Default Scene in code -- used when scene serialization is changed
		//DefaultScene();
		PhysicsScene();
		//ProceduralScene();

		// Load Scene -- normal behaviour
		//m_sceneData->LoadAndInit();
		//m_sceneData->Save();

		running = true;
		m_playState = PlayState::STOPPED;

		// Initialize Subsystems
		for (auto& [fst, snd] : m_subsystemsPriority)
		{
			m_subsystems[snd]->Init();
		}

		// Initialize Systems
		{
			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::FixedUpdate);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				system->Init();
				system->PreStart();
			}
		}

		{
			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::Update);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				system->Init();
				system->PreStart();
			}
		}

		{
			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::PreRender);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				system->Init();
				system->PreStart();
			}
		}

		{
			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::Render);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				system->Init();
				system->PreStart();
			}
		}

		m_lastTime = glfwGetTime(); // Time Count Started
		m_currentTime = m_lastTime;
	}

	bool Engine::Update()
	{
		// Run Game Loop;
		m_lastTime = m_currentTime;
		m_currentTime = glfwGetTime();
		m_deltaTime = m_currentTime - m_lastTime;
		m_idleTime = 0.0;

		if (m_frameRateMax > 0)
		{
			const double deltaTimeMax = 1.0 / m_frameRateMax;

			// Sleep until next frame should start
			auto idleStartTime = glfwGetTime();

			while (m_deltaTime < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				m_currentTime = glfwGetTime();
				m_deltaTime = m_currentTime - m_lastTime;
			}

			auto idleEndTime = glfwGetTime();
			m_idleTime = idleEndTime - idleStartTime;
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (m_deltaTime > m_timeStepLimit)
		{
			m_deltaTime = m_timeStepLimit;
		}

		auto ecsWorld = GetSubsystem<ECS::World>();
		auto audioSubsystem = GetSubsystem<Audio::AudioSubsystem>();

		// Update all Subsystems
		for (auto& [fst, snd] : m_subsystems)
		{
			if (snd->ShouldUpdate())
			{
				snd->Update();
			}
		}

		auto inputSubsystem = GetSubsystem<Input::InputSubsystem>();
		if (inputSubsystem->GetAction("Play").state == Input::KeyState::PRESSED)
		{
			Play();
		}

		if (inputSubsystem->GetAction("Restart").state == Input::KeyState::PRESSED)
		{
			Restart();
		}

		// Call system start functions to prepare for gameplay
		if (m_playState == PlayState::STARTED)
		{
			for (auto& system : m_systems)
			{
				system->Start();
			}

			// Get Snapshot of current scene data
			m_sceneData->UpdateData();

			m_accumulatedTime = 0.0;
			m_playState = PlayState::PLAYING;
		}

		if (m_playState == PlayState::JUST_PAUSED)
		{
			audioSubsystem->PauseAllSounds();

			m_playState = PlayState::PAUSED;
		}

		if (m_playState == PlayState::JUST_UNPAUSED)
		{
			audioSubsystem->PlayAllSounds();

			m_playState = PlayState::PLAYING;
		}

		if (m_playState == PlayState::PLAYING)
		{
			// Fixed Update
			{
				// Add onto accumulated time
				m_accumulatedTime += m_deltaTime;

				auto stageStartTime = glfwGetTime();

				// Perform system updates until simulation is caught up
				while (m_accumulatedTime >= m_timeStepFixed)
				{
					m_accumulatedTime -= m_timeStepFixed;

					// FixedUpdate Systems
					auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::FixedUpdate);
					for (auto& it = itr.first; it != itr.second; ++it)
					{
						std::shared_ptr<ECS::System> system = it->second;

						auto startTime = glfwGetTime();

						system->Update();

						auto endTime = glfwGetTime();

						m_systemExecutionTime[Core::UpdateOrder::FixedUpdate][system->GetInfo().name] = endTime - startTime;
					}
				}

				auto stageEndTime = glfwGetTime();

				m_stageExecutionTime[Core::UpdateOrder::FixedUpdate] = stageEndTime - stageStartTime;
			}

			// Update
			{
				auto stageStartTime = glfwGetTime();

				auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::Update);
				for (auto& it = itr.first; it != itr.second; ++it)
				{
					std::shared_ptr<ECS::System> system = it->second;

					auto startTime = glfwGetTime();

					system->Update();

					auto endTime = glfwGetTime();

					m_systemExecutionTime[Core::UpdateOrder::Update][system->GetInfo().name] = endTime - startTime;
				}

				auto stageEndTime = glfwGetTime();

				m_stageExecutionTime[Core::UpdateOrder::Update] = stageEndTime - stageStartTime;
			}
		}

		// UI
		if (m_shouldRenderEditorUI)
		{
			m_uiManager->Update();
			m_uiManager->DrawUI(m_deltaTime);
		}

		// PreRender
		{
			auto stageStartTime = glfwGetTime();

			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::PreRender);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				auto startTime = glfwGetTime();

				system->Update();

				auto endTime = glfwGetTime();

				m_systemExecutionTime[Core::UpdateOrder::PreRender][system->GetInfo().name] = endTime - startTime;
			}

			auto stageEndTime = glfwGetTime();

			m_stageExecutionTime[Core::UpdateOrder::Render] = stageEndTime - stageStartTime;
		}

		// Render
		{
			auto stageStartTime = glfwGetTime();

			auto itr = m_systemUpdateVectors.equal_range(Core::UpdateOrder::Render);
			for (auto& it = itr.first; it != itr.second; ++it)
			{
				std::shared_ptr<ECS::System> system = it->second;

				auto startTime = glfwGetTime();

				system->Update();

				auto endTime = glfwGetTime();

				m_systemExecutionTime[Core::UpdateOrder::Render][system->GetInfo().name] = endTime - startTime;
			}

			auto stageEndTime = glfwGetTime();

			m_stageExecutionTime[Core::UpdateOrder::Render] = stageEndTime - stageStartTime;
		}

		if (m_playState == PlayState::JUST_STOPPED)
		{
			// Cleanup Systems and ECS
			for (auto system : m_systems)
			{
				system->Stop();
			}
			ecsWorld->Reset();

			// Re-Initialize Systems and ECS
			m_sceneData->Init();

			// Perform Pre-Gameplay Initialization on Systems
			for (auto system : m_systems)
			{
				system->PreStart();
			}

			audioSubsystem->StopAllSounds();

			m_accumulatedTime = 0.0;
			m_playState = PlayState::STOPPED;
		}

		auto windowSubsystem = GetSubsystem<Window::WindowSubsystem>();
		if (windowSubsystem->ShouldPrimaryWindowClose())
		{
			running = false;
		}

		return running;
	}

	void Engine::Destroy()
	{
		// Cleanup All Systems
		for (auto system : m_systems)
		{
			system->Cleanup();
		}

		m_systems.clear();

		// Cleanup all subsystems
		for (auto& [fst, snd] : m_subsystems)
		{
			snd->Destroy();
			snd = nullptr;
		}

		m_subsystems.clear();

		// Cleanup UI Manager
		if (m_shouldRenderEditorUI)
		{
			m_uiManager->Cleanup();
			m_uiManager = nullptr;
		}

		while (true)
		{
			if (JobSystem::Get()->Wait())
			{
				break;
			}
		}

		JobSystem::Get()->Stop();
		JobSystem::Clear();

		// Clear Asset Registry
		Assets::AssetRegistry::Clear();
	}

	void Engine::AddDefaultAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::SoundAsset>(soundPath1)->ID();
	}

	void Engine::ReimportDefaultAssets()
	{
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\chalet.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\cube.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\space_engineer.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\Sphere.dae");
	}

	void Engine::LoadAndResaveAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const auto meshAsset1 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId1));
		const auto meshAsset2 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId2));
		const auto meshAsset3 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId3));
		const auto meshAsset4 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId4));

		meshAsset1->Load();
		meshAsset2->Load();
		meshAsset3->Load();
		meshAsset4->Load();

		meshAsset1->Save();
		meshAsset2->Save();
		meshAsset3->Save();
		meshAsset4->Save();
	}

	void Engine::DefaultScene()
	{
		auto ecsWorld = GetSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::SoundAsset>(soundPath1)->ID();

		const int numEntities = 7;
		std::vector<std::shared_ptr<ECS::Entity>> entities;
		entities.reserve(numEntities);

		std::string names[numEntities] = { "House", "Sphere", "Falling Cube", "Dir Light", "Static Cube", "Plane", "Spot Light" };

		// Add Default Scene Components to ECS
		for (int i = 0; i < numEntities; i++)
		{
			const auto entity = ECS::CreateEntity(ecsWorld);
			entity->SetName(names[i]);
			entity->AddComponent<TransformComponent>();
			entity->AddComponent<Rendering::MeshComponent>();
			entities.push_back(entity);
		}

		// Initialize Components with default values
		entities[0]->GetComponent<TransformComponent>() = { Vector3f(2.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[1]->GetComponent<TransformComponent>() = { Vector3f(-1.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[2]->GetComponent<TransformComponent>() = { Vector3f(0.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[3]->GetComponent<TransformComponent>() =
		{
			Vector3f(-5.0f, 0.0f, 0.0f),
			Maths::Quat(.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};
		entities[4]->GetComponent<TransformComponent>() = { Vector3f(-1.75f, -5.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[5]->GetComponent<TransformComponent>() = { Vector3f(0.0f, -10.0f, 0.0f), Maths::Quat(), Vector3f(10.0f, 1.0f, 10.0f) };
		entities[6]->GetComponent<TransformComponent>() =
		{
			Vector3f(5.0f, 0.0f, 0.0f),
			Maths::Quat(-.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};

		entities[0]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId1;
		entities[0]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId1;

		entities[1]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId2;
		entities[1]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		entities[2]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[2]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[3]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[3]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[4]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[4]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[5]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[5]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[6]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[6]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		// Setup Light Component
		entities[3]->AddComponent<Rendering::LightComponent>();
		entities[3]->GetComponent<Rendering::LightComponent>().color = Vector3f(1.f, 1.f, 1.f);
		entities[3]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::DIRECTIONAL;
		//entities[3]->AddComponent<Rendering::ShadowCasterComponent>();

		entities[6]->AddComponent<Rendering::LightComponent>();
		entities[6]->GetComponent<Rendering::LightComponent>().color = Vector3f(0.f, 0.f, 1.f);
		entities[6]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::SPOT;

		auto& script = entities[0]->AddAndGetComponent<Scripting::AngelScriptComponent>();
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::PhysicsScene()
	{
		auto ecsWorld = GetSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::SoundAsset>(soundPath1)->ID();

		// Create Light Entity
		const auto lightEntity = ECS::CreateEntity(ecsWorld);

		lightEntity->SetName("Light");

		lightEntity->AddComponent<TransformComponent>();
		lightEntity->GetComponent<TransformComponent>() =
		{
			Vector3f(-5.0f, 0.0f, 0.0f),
			Maths::Quat(.5f, -0.5f, 0.0f),
			Vector3f(1.0f)
		};

		auto& lightComp = lightEntity->AddAndGetComponent<Rendering::LightComponent>();
		lightComp.type = Rendering::LightType::DIRECTIONAL;
		lightComp.color = glm::vec3(1.0f, 1.0f, 1.0f);

		// Create Floor Entity
		const auto floorEntity = ECS::CreateEntity(ecsWorld);

		floorEntity->SetName("Floor");

		floorEntity->AddComponent<TransformComponent>();
		floorEntity->AddComponent<Rendering::MeshComponent>();
		floorEntity->AddComponent<Physics::RigidbodyComponent2D>();
		floorEntity->AddComponent<Physics::BoxComponent2D>();

		floorEntity->GetComponent<TransformComponent>() = { Vector3f(0.0f), Maths::Quat(), Vector3f(250.0f, 1.0f, 1.0f) };
		floorEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		floorEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		floorEntity->GetComponent<Physics::BoxComponent2D>().halfExtent = Vector2f(250.0f, 1.0f);

		const float xOffset = 400.0f;
		const Vector3f startPosition(-xOffset, 10.f, 0.f);
		const Vector3f endPosition(xOffset, 10.f, 0.f);
		const int numBodies = 5000;
		Vector3f positionOffset = endPosition - startPosition;
		positionOffset.x /= numBodies;
		for (int i = 0; i < numBodies; i++)
		{
			const auto entity = ECS::CreateEntity(ecsWorld);

			entity->SetName("Box " + std::to_string(i + 1));
			
			entity->AddComponent<TransformComponent>();
			entity->AddComponent<Rendering::MeshComponent>();
			entity->AddComponent<Physics::RigidbodyComponent2D>();
			entity->AddComponent<Physics::BoxComponent2D>();

			Vector3f position = startPosition + (positionOffset * (float)i);
			entity->GetComponent<TransformComponent>() = { position, Maths::Quat(), Vector3f(1.0f) };
			entity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
			entity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
			entity->GetComponent<Physics::RigidbodyComponent2D>().invMass = 1.0f;
			entity->GetComponent<Physics::RigidbodyComponent2D>().elasticity = 1.0f;
			entity->GetComponent<Physics::RigidbodyComponent2D>().bodyType = Physics::BodyType::Dynamic;
			entity->GetComponent<Physics::BoxComponent2D>().halfExtent = Vector2f(1.0f, 1.0f);
		}

		//// Create Box Entity
		//const auto boxEntity = ECS::CreateEntity(ecsWorld);

		//boxEntity->SetName("Box");

		//boxEntity->AddComponent<TransformComponent>();
		//boxEntity->AddComponent<Rendering::MeshComponent>();
		//boxEntity->AddComponent<Physics::RigidbodyComponent2D>();
		//boxEntity->AddComponent<Physics::BoxComponent2D>();
		//boxEntity->AddComponent<Scripting::AngelScriptComponent>();

		//boxEntity->GetComponent<TransformComponent>() = { Vector3f(0.0f, 2.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };
		//boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		//boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		//boxEntity->GetComponent<Physics::RigidbodyComponent2D>().invMass = 0.5f;
		//boxEntity->GetComponent<Physics::RigidbodyComponent2D>().elasticity = 0.5f;
		//boxEntity->GetComponent<Physics::RigidbodyComponent2D>().bodyType = Physics::BodyType::Dynamic;
		//boxEntity->GetComponent<Physics::BoxComponent2D>().halfExtent = Vector2f(1.0f, 1.0f);
		//boxEntity->GetComponent<Scripting::AngelScriptComponent>().name = "PhysicsScript";
		//boxEntity->GetComponent<Scripting::AngelScriptComponent>().dir = contentRootPath / "scripts\\Physics.pscript";

		//// Create Circle Entity
		//const auto circleEntity = ECS::CreateEntity(ecsWorld);

		//circleEntity->SetName("Circle");

		//circleEntity->AddComponent<TransformComponent>();
		//circleEntity->AddComponent<Rendering::MeshComponent>();
		//circleEntity->AddComponent<Physics::RigidbodyComponent2D>();
		//circleEntity->AddComponent<Physics::BoxComponent2D>();

		//circleEntity->GetComponent<TransformComponent>() = { Vector3f(0.0f, 10.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };
		//circleEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		//circleEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		//circleEntity->GetComponent<Physics::RigidbodyComponent2D>().invMass = 1.0f;
		//circleEntity->GetComponent<Physics::RigidbodyComponent2D>().elasticity = 0.75f;
		//circleEntity->GetComponent<Physics::RigidbodyComponent2D>().bodyType = Physics::BodyType::Dynamic;
		//circleEntity->GetComponent<Physics::BoxComponent2D>().halfExtent = Vector2f(1.0f, 1.0f);
	}

	void Engine::ProceduralScene()
	{
		auto ecsWorld = GetSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& cubeMeshPath = "meshes\\cube.pstaticmesh";

		UUID cubeMeshId = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(cubeMeshPath)->ID();

		const fs::path& cubeTexturePath = "textures\\cube.ptexture";

		UUID cubeTextureId = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(cubeTexturePath)->ID();

		const auto lightEntity = ECS::CreateEntity(ecsWorld);
		lightEntity->SetName("Light");
		lightEntity->AddComponent<TransformComponent>();
		lightEntity->GetComponent<TransformComponent>().position = { 0.0, 10.0, 0.0 };
		lightEntity->GetComponent<TransformComponent>().scale = { 0.25f };
		lightEntity->AddComponent<Rendering::LightComponent>();
		lightEntity->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::DIRECTIONAL;
		lightEntity->AddComponent<Rendering::MeshComponent>();
		lightEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		lightEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;
		//lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		const auto planeEntity = ECS::CreateEntity(ecsWorld);
		planeEntity->SetName("Terrain");
		planeEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, -10.0f, 0.0 };
		planeEntity->AddAndGetComponent<Rendering::ProceduralMeshComponent>().textureAssetID = cubeTextureId;
		planeEntity->AddComponent<Procedural::TerrainComponent>();
		planeEntity->GetComponent<Procedural::TerrainComponent>().halfSize = { 50 };
		planeEntity->GetComponent<Procedural::TerrainComponent>().numQuads = { 50 };
		planeEntity->GetComponent<Procedural::TerrainComponent>().heightMultiplier = 10;

		const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		sphereEntity->SetName("Sphere");
		sphereEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, 5.0, 0.0 };
		sphereEntity->AddAndGetComponent<Rendering::ProceduralMeshComponent>().textureAssetID = cubeTextureId;
		sphereEntity->AddComponent<Procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;*/
	}

	void Engine::Play()
	{
		switch (m_playState)
		{
		case PlayState::STOPPED:
			m_playState = PlayState::STARTED;
			break;
		case PlayState::PLAYING:
			m_playState = PlayState::JUST_PAUSED;
			break;
		case PlayState::PAUSED:
			m_playState = PlayState::JUST_UNPAUSED;
			break;
		}
	}

	void Engine::Restart()
	{
		if (m_playState == PlayState::PLAYING || m_playState == PlayState::PAUSED || m_playState == PlayState::STOPPED)
		{
			m_playState = PlayState::JUST_STOPPED;
		}
	}

	void Engine::Exit()
	{
		running = false;
	}
}