#include "Core/Engine.h"

#include <chrono>
#include <thread>

//#include "Rendering/BGFX/BGFXRenderSystem.h"
//#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/ShaderAsset.h"
#include "Assets/MaterialAsset.h"
#include "Assets/SoundAsset.h"
#include "Assets/TextureAsset.h"
#include "Audio/AudioSubsystem.h"
#include "Components/TransformComponent.h"
#include "Components/Procedural/ProceduralMeshComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components/Scripting/AngelScriptComponent.h"
#include "Core/EnkiTSSubsystem.h"
#include "Core/SceneSubsystem.h"
#include "Core/SignalSubsystem.h"
#include "ECS/EnTTSubsystem.h"
#include "Input/InputSubsystem.h"
#include "Physics/Onager2D/OnagerPhysicsSystem2D.h"
#include "Procedural/ProceduralMeshGenSystem.h"
#include "Rendering/Vulkan/VKRenderSystem.h"
#include "Scripting/AngelScriptSystem.h"
#include "UI/Editor/UISubsystem.h"
#include "Window/WindowSubsystem.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace puffin::core
{
	void Engine::init()
	{
		// Subsystems

		auto windowSubsystem = registerSubsystem<window::WindowSubsystem>();
		auto signalSubsystem = registerSubsystem<SignalSubsystem>();
		auto enkitsSubsystem = registerSubsystem<EnkiTSSubsystem>();
		auto inputSubsystem = registerSubsystem<input::InputSubsystem>();
		auto audioSubsystem = registerSubsystem<audio::AudioSubsystem>();
		auto enttSubsystem = registerSubsystem<ecs::EnTTSubsystem>();
		auto uiSubsystem = registerSubsystem<ui::UISubsystem>();
		auto sceneSubsystem = registerSubsystem<io::SceneSubsystem>();

		// Load Project File
		const auto projectPath = fs::path(R"(C:\Projects\PuffinProject\Puffin.pproject)");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		LoadProject(projectPath, mProjectFile);

		// Load Default Scene (if set)
		fs::path defaultScenePath = projectDirPath.parent_path() / "content" / mProjectFile.defaultScenePath;

		auto sceneData = sceneSubsystem->createScene(defaultScenePath);

		// Register Components to ECS World and Scene Data Class
		sceneData->registerComponent<SceneObjectComponent>();
		sceneData->registerComponent<TransformComponent>();
		sceneData->registerComponent<rendering::MeshComponent>();
		sceneData->registerComponent<rendering::LightComponent>();
		sceneData->registerComponent<rendering::ShadowCasterComponent>();
		sceneData->registerComponent<rendering::CameraComponent>();
		sceneData->registerComponent<physics::RigidbodyComponent2D>();
		sceneData->registerComponent<physics::BoxComponent2D>();
		sceneData->registerComponent<physics::CircleComponent2D>();
		sceneData->registerComponent<scripting::AngelScriptComponent>();
		sceneData->registerComponent<rendering::ProceduralMeshComponent>();
		sceneData->registerComponent<procedural::PlaneComponent>();
		sceneData->registerComponent<procedural::TerrainComponent>();
		sceneData->registerComponent<procedural::IcoSphereComponent>();

		// Systems
		//registerSystem<Rendering::BGFX::BGFXRenderSystem>();
		registerSystem<rendering::VKRenderSystem>();
		registerSystem<physics::OnagerPhysicsSystem2D>();
		//registerSystem<Physics::Box2DPhysicsSystem>();
		registerSystem<scripting::AngelScriptSystem>();
		registerSystem<procedural::ProceduralMeshGenSystem>();

		// Register Assets
		assets::AssetRegistry::get()->registerAssetType<assets::StaticMeshAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::TextureAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::SoundAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::ShaderAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::MaterialAsset>();

		// Load Asset Cache
		assets::AssetRegistry::get()->setProjectName(mProjectFile.name);
		assets::AssetRegistry::get()->setProjectRoot(projectDirPath);

		// Load Project Settings
		io::LoadSettings(projectDirPath.parent_path() / "Settings.json", mSettings);

		// Load/Initialize Assets
		assets::AssetRegistry::get()->loadAssetCache();
		//addDefaultAssets();
		//assets::AssetRegistry::get()->saveAssetCache();
		//loadAndResaveAssets();
		//ReimportDefaultAssets();

		if (constexpr bool setupDefaultScene = false; setupDefaultScene)
		{
			// Create Default Scene in code -- used when scene serialization is changed
			//defaultScene();
			physicsScene();
			//proceduralScene();

			sceneData->updateData(enttSubsystem);
			sceneData->save();
			sceneData->clear();
		}

		mRunning = true;
		mPlayState = PlayState::Stopped;

		// Initialize Systems
		{
			executeCallbacks(ExecutionStage::Init);
		}

		mLastTime = glfwGetTime(); // Time Count Started
		mCurrentTime = mLastTime;
	}

	bool Engine::update()
	{
		// Run Game Loop;
		mLastTime = mCurrentTime;
		mCurrentTime = glfwGetTime();
		mDeltaTime = mCurrentTime - mLastTime;

		updateExecutionTime();

		if (mFrameRateMax > 0)
		{
			const double deltaTimeMax = 1.0 / mFrameRateMax;
			double idleStartTime = 0.0;

			if (mShouldTrackExecutionTime)
			{
				// Sleep until next frame should start
				idleStartTime = glfwGetTime();
			}

			while (mDeltaTime < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				mCurrentTime = glfwGetTime();
				mDeltaTime = mCurrentTime - mLastTime;
			}

			if (mShouldTrackExecutionTime)
			{
				const double idleEndTime = glfwGetTime();

				mStageExecutionTime[ExecutionStage::Idle] = idleEndTime - idleStartTime;
			}
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (mDeltaTime > mTimeStepLimit)
		{
			mDeltaTime = mTimeStepLimit;
		}

		const auto audioSubsystem = getSubsystem<audio::AudioSubsystem>();

		// Update all Subsystems
		{
			executeCallbacks(ExecutionStage::SubsystemUpdate, true);
		}

		const auto inputSubsystem = getSubsystem<input::InputSubsystem>();
		if (inputSubsystem->getAction("Play").state == input::KeyState::Pressed)
		{
			play();
		}

		if (inputSubsystem->getAction("Restart").state == input::KeyState::Pressed)
		{
			restart();
		}

		// Call system start functions to prepare for gameplay
		if (mPlayState == PlayState::Started)
		{
			executeCallbacks(ExecutionStage::BeginPlay);

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::JustPaused)
		{
			audioSubsystem->pauseAllSounds();

			mPlayState = PlayState::Paused;
		}

		if (mPlayState == PlayState::JustUnpaused)
		{
			audioSubsystem->playAllSounds();

			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::Playing)
		{
			// Fixed Update
			{
				// Add onto accumulated time
				mAccumulatedTime += mDeltaTime;

				while (mAccumulatedTime >= mTimeStepFixed)
				{
					mAccumulatedTime -= mTimeStepFixed;

					executeCallbacks(ExecutionStage::FixedUpdate, true);
				}
			}

			// Update
			{
				executeCallbacks(ExecutionStage::Update, true);
			}
		}

		// Render
		{
			executeCallbacks(ExecutionStage::Render, true);
		}

		if (mPlayState == PlayState::JustStopped)
		{
			// Cleanup Systems and ECS
			executeCallbacks(ExecutionStage::EndPlay);

			audioSubsystem->stopAllSounds();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		if (const auto windowSubsystem = getSubsystem<window::WindowSubsystem>(); windowSubsystem->shouldPrimaryWindowClose())
		{
			mRunning = false;
		}

		return mRunning;
	}

	void Engine::destroy()
	{
		// Cleanup All Systems
		executeCallbacks(ExecutionStage::Shutdown);

		mSystems.clear();
		mSubsystems.clear();

		// Clear Asset Registry
		assets::AssetRegistry::clear();
	}

	void Engine::addDefaultAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		PuffinID meshId1 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath1)->id();
		PuffinID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		PuffinID meshId3 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath3)->id();
		PuffinID meshId4 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		PuffinID textureId1 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath1)->id();
		PuffinID textureId2 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->addAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path shaderPath1 = "shaders\\forward_shading\\forward_shading_vert.pshader";
		const fs::path shaderPath2 = "shaders\\forward_shading\\forward_shading_frag.pshader";

		const auto shaderAsset1 = assets::AssetRegistry::get()->addAsset<assets::ShaderAsset>(shaderPath1);
		const auto shaderAsset2 = assets::AssetRegistry::get()->addAsset<assets::ShaderAsset>(shaderPath2);

		shaderAsset1->setType(assets::ShaderType::Vertex);

		shaderAsset1->setShaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.vert)"));
		shaderAsset1->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_vs.spv)"));
		//shaderAsset1->loadCodeFromBinary();
		shaderAsset1->save();

		shaderAsset2->setType(assets::ShaderType::Fragment);
		shaderAsset2->setShaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.frag)"));
		shaderAsset2->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_fs.spv)"));
		//shaderAsset2->loadCodeFromBinary();
		shaderAsset2->save();

		const fs::path materialPath1 = "shaders\\forward_shading\\forward_shading_default.pmaterial";
		const fs::path materialPath2 = "shaders\\forward_shading\\forward_shading_chalet.pmaterial";

		const auto materialAsset1 = assets::AssetRegistry::get()->addAsset<assets::MaterialAsset>(materialPath1);
		const auto materialAsset2 = assets::AssetRegistry::get()->addAsset<assets::MaterialAsset>(materialPath2);

		/*materialAsset1->setVertexShaderID(shaderAsset1->id());
		materialAsset1->setFragmentShaderID(shaderAsset2->id());
		materialAsset1->getTexIDs()[0] = textureId1;

		materialAsset1->save();

		materialAsset2->setVertexShaderID(shaderAsset1->id());
		materialAsset2->setFragmentShaderID(shaderAsset2->id());
		materialAsset2->setBaseMaterialID(materialAsset1->id());
		materialAsset2->getTexIDs()[0] = textureId2;
		materialAsset2->getTexIDOverride()[0] = true;

		materialAsset2->save();*/
	}

	void Engine::reimportDefaultAssets()
	{
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\chalet.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\cube.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\space_engineer.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\Sphere.dae");
	}

	void Engine::loadAndResaveAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		std::vector meshPaths = { meshPath1, meshPath2, meshPath3, meshPath4 };

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		std::vector texturePaths = { texturePath1, texturePath2 };

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		const fs::path shaderPath1 = "shaders\\forward_shading\\forward_shading_vert.pshader";
		const fs::path shaderPath2 = "shaders\\forward_shading\\forward_shading_frag.pshader";

		const fs::path materialPath1 = "shaders\\forward_shading\\forward_shading_default.pmaterial";
		const fs::path materialPath2 = "shaders\\forward_shading\\forward_shading_chalet.pmaterial";

		std::vector paths =
		{
			meshPath1, meshPath2, meshPath3, meshPath4,
			texturePath1, texturePath2,
			shaderPath1, shaderPath2,
			materialPath1, materialPath2
		};

		for (const auto path : paths)
		{
			const auto asset = assets::AssetRegistry::get()->getAsset(path);
			asset->load();
			asset->save();
			asset->unload();
		}
	}

	void Engine::defaultScene()
	{
		// Initialize Assets
		fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath1)->id();
		const PuffinID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		const PuffinID meshId3 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath3)->id();
		const PuffinID meshId4 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		const PuffinID textureId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath1)->id();
		const PuffinID textureId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialPath1 = "shaders\\forward_shading\\forward_shading_default.pmaterial";
		const fs::path materialPath2 = "shaders\\forward_shading\\forward_shading_chalet.pmaterial";

		PuffinID materialId1 = assets::AssetRegistry::get()->getAsset<assets::MaterialAsset>(materialPath1)->id();
		PuffinID materialId2 = assets::AssetRegistry::get()->getAsset<assets::MaterialAsset>(materialPath2)->id();

		const auto enttSubsystem = getSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->registry();

		constexpr int numEntities = 7;
		std::vector<entt::entity> entities;
		entities.reserve(numEntities);

		const std::string names[numEntities] = { "House", "Sphere", "Falling Cube", "Dir Light", "Static Cube", "Plane", "Spot Light" };
		const Vector3d positions[numEntities] =
		{
			Vector3d(2.0, 0.0, 0.0),
			Vector3d(-1.0, 0.0, 0.0),
			Vector3d(0.0),
			Vector3d(-5.0, 0.0, 0.0),
			Vector3d(-1.75, -5.0, 0.0),
			Vector3d(0.0, -10.0, 0.0),
			Vector3d(10.0, 5.0, 0.0)
		};

		const maths::Quat orientations[numEntities] =
		{
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
			maths::Quat(angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0))),
		};

		const Vector3f scales[numEntities] =
		{
			Vector3f(1.0f),
			Vector3f(1.0f),
			Vector3f(1.0f),
			Vector3f(.25f),
			Vector3f(1.0f),
			Vector3f(10.0f, 1.0f, 10.0f),
			Vector3f(0.25f)
		};

		const PuffinID meshIDs[numEntities] = { meshId1, meshId2, meshId3, meshId3, meshId3, meshId3, meshId3 };
		const PuffinID materialIDs[numEntities] = { materialId2, materialId1, materialId1, materialId1, materialId1, materialId1, materialId1 };

		// Add Default Scene Components to ECS
		for (int i = 0; i < numEntities; i++)
		{
			const auto entity = enttSubsystem->createEntity(names[i]);

			registry->emplace<TransformComponent>(entity, positions[i], orientations[i], scales[i]);
			registry->emplace<rendering::MeshComponent>(entity, meshIDs[i], materialIDs[i]);

			entities.push_back(entity);
		}

		// Setup Light Component

		auto& dirLight = registry->emplace<rendering::LightComponent>(entities[3]);
		dirLight.color = Vector3f(0.f, 0.f, 0.f);
		dirLight.type = rendering::LightType::Directional;
		dirLight.ambientIntensity = 0.f;

		//registry->emplace<rendering::ShadowCasterComponent>(entities[3]);

		auto& spotLight = registry->emplace<rendering::LightComponent>(entities[6]);
		spotLight.color = Vector3f(1.f, 0.f, 0.f);
		spotLight.type = rendering::LightType::Spot;
		spotLight.direction = Vector3f(-0.5f, -0.5f, 0.0f);
		spotLight.ambientIntensity = 0.f;

		/*auto& script = registry->emplace<scripting::AngelScriptComponent>(entities[0]);
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";*/
	}

	void Engine::physicsScene()
	{
		// Initialize Assets
		fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath1)->id();
		const PuffinID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		const PuffinID meshId3 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath3)->id();
		const PuffinID meshId4 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path materialPath1 = "shaders\\forward_shading\\forward_shading_default.pmaterial";
		const fs::path materialPath2 = "shaders\\forward_shading\\forward_shading_chalet.pmaterial";

		PuffinID materialId1 = assets::AssetRegistry::get()->getAsset<assets::MaterialAsset>(materialPath1)->id();
		PuffinID materialId2 = assets::AssetRegistry::get()->getAsset<assets::MaterialAsset>(materialPath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const auto enttSubsystem = getSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->registry();

		// Create Light Entity
		{
			const auto lightEntity = enttSubsystem->createEntity("Light");

			auto& transform = registry->emplace<TransformComponent>(lightEntity);
			transform.position = Vector3f(-5.0f, 0.0f, 0.0f);
			transform.orientation = glm::angleAxis(0.0f, glm::vec3(.5f, -0.5f, 0.0f));

			auto& light = registry->emplace<rendering::LightComponent>(lightEntity);
			light.type = rendering::LightType::Directional;
			light.color = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		constexpr int numBodies = 1000;
		constexpr float xOffset = numBodies * 2.0f;
		constexpr std::array<float, 4> yOffsets = { 20.0f, 40.0f, 60.0f, 80.0f };

		// Create Floor Entity
		{
			const auto floorEntity = enttSubsystem->createEntity("Floor");

			auto& transform = registry->emplace<TransformComponent>(floorEntity);
			transform.scale = Vector3f(xOffset, 1.0f, 1.0f);

			registry->emplace<rendering::MeshComponent>(floorEntity, meshId3, materialId2);

			registry->emplace<physics::BoxComponent2D>(floorEntity, Vector2f(xOffset, 1.0f));

			registry->emplace<physics::RigidbodyComponent2D>(floorEntity);
		}

		// Create Box Entities
		{
			const Vector3f startPosition(-xOffset, 0.f, 0.f);
			const Vector3f endPosition(xOffset, 0.f, 0.f);

			Vector3f positionOffset = endPosition - startPosition;
			positionOffset.x /= numBodies;

			for (int i = 0; i < numBodies; i++)
			{
				const std::string name = "Box";
				const auto boxEntity = enttSubsystem->createEntity(name);

				Vector3f position = startPosition + positionOffset * static_cast<float>(i);

				const int yIdx = i % yOffsets.size();
				position.y = yOffsets[yIdx];

				registry->emplace<TransformComponent>(boxEntity, position);

				registry->emplace<rendering::MeshComponent>(boxEntity, meshId3, materialId2);

				registry->emplace<physics::BoxComponent2D>(boxEntity, Vector2f(1.0f));

				registry->emplace<physics::RigidbodyComponent2D>(boxEntity, physics::BodyType::Dynamic, 1.0f);
			}
		}
	}

	void Engine::proceduralScene()
	{
		//auto ecsWorld = getSubsystem<ECS::World>();

		//// Initialize Assets
		//fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		//const fs::path& cubeMeshPath = "meshes\\cube.pstaticmesh";

		//PuffinId cubeMeshId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeMeshPath)->id();

		//const fs::path& cubeTexturePath = "textures\\cube.ptexture";

		//PuffinId cubeTextureId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeTexturePath)->id();

		//const auto lightEntity = ECS::CreateEntity(ecsWorld);
		//lightEntity->SetName("Light");
		//lightEntity->AddComponent<TransformComponent>();
		//lightEntity->GetComponent<TransformComponent>().position = { 0.0, 10.0, 0.0 };
		//lightEntity->GetComponent<TransformComponent>().scale = { 0.25f };
		//lightEntity->AddComponent<rendering::LightComponent>();
		//lightEntity->GetComponent<rendering::LightComponent>().type = rendering::LightType::Directional;
		//lightEntity->AddComponent<rendering::MeshComponent>();
		//lightEntity->GetComponent<rendering::MeshComponent>().meshAssetId = cubeMeshId;
		//lightEntity->GetComponent<rendering::MeshComponent>().textureAssetId = cubeTextureId;
		////lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		//const auto planeEntity = ECS::CreateEntity(ecsWorld);
		//planeEntity->SetName("Terrain");
		//planeEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, -10.0f, 0.0 };
		//planeEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		//planeEntity->AddComponent<procedural::TerrainComponent>();
		//planeEntity->GetComponent<procedural::TerrainComponent>().halfSize = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().numQuads = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().heightMultiplier = 10;

		//const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		//sphereEntity->SetName("Sphere");
		//sphereEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, 5.0, 0.0 };
		//sphereEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		//sphereEntity->AddComponent<procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;*/
	}

	void Engine::play()
	{
		switch (mPlayState)
		{
		case PlayState::Stopped:
			mPlayState = PlayState::Started;
			break;
		case PlayState::Playing:
			mPlayState = PlayState::JustPaused;
			break;
		case PlayState::Paused:
			mPlayState = PlayState::JustUnpaused;
			break;
		}
	}

	void Engine::restart()
	{
		if (mPlayState == PlayState::Playing || mPlayState == PlayState::Paused || mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::JustStopped;
		}
	}

	void Engine::exit()
	{
		mRunning = false;
	}
}