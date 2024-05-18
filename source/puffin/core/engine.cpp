#include "puffin/core/engine.h"

#include <chrono>
#include <thread>

#include "puffin/assets/asset_importers.h"
#include "puffin/assets/asset_registry.h"
#include "puffin/assets/material_asset.h"
#include "puffin/assets/mesh_asset.h"
#include "puffin/assets/shader_asset.h"
#include "puffin/assets/sound_asset.h"
#include "puffin/assets/texture_asset.h"
#include "puffin/audio/audio_subsystem.h"
#include "puffin/components/physics/2d/rigidbody_component_2d.h"
#include "puffin/components/physics/2d/shape_components_2d.h"
#include "puffin/components/physics/3d/rigidbody_component_3d.h"
#include "puffin/components/physics/3d/shape_components_3d.h"
#include "puffin/components/procedural/procedural_mesh_component.h"
#include "puffin/components/rendering/camera_component.h"
#include "puffin/components/rendering/light_component.h"
#include "puffin/components/rendering/mesh_component.h"
#include "puffin/components/scripting/angelscript_component.h"
#include "puffin/core/enkits_subsystem.h"
#include "puffin/core/scene_subsystem.h"
#include "puffin/core/signal_subsystem.h"
#include "puffin/input/input_subsystem.h"
#include "puffin/nodes/physics/rigidbody_node_3d.h"
#include "puffin/nodes/rendering/light_node_3d.h"
#include "puffin/nodes/rendering/mesh_node.h"
#include "puffin/scene/scene_graph.h"
#include "puffin/ui/editor/ui_subsystem.h"
#include "puffin/window/window_subsystem.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace puffin::core
{
	void Engine::setup(const fs::path& projectPath)
	{
		// Subsystems
		auto windowSubsystem = registerSystem<window::WindowSubsystem>();
		auto signalSubsystem = registerSystem<SignalSubsystem>();
		auto enkitsSubsystem = registerSystem<EnkiTSSubsystem>();
		auto inputSubsystem = registerSystem<input::InputSubsystem>();
		auto audioSubsystem = registerSystem<audio::AudioSubsystem>();
		auto enttSubsystem = registerSystem<ecs::EnTTSubsystem>();
		auto uiSubsystem = registerSystem<ui::UISubsystem>();
		auto sceneSubsystem = registerSystem<io::SceneSubsystem>();
		auto scene_graph = registerSystem<scene::SceneGraph>();

		scene_graph->register_default_node_types();

		// Load Project File
		LoadProject(projectPath, mProjectFile);

		// Setup asset registry
		assets::AssetRegistry::get()->init(mProjectFile, projectPath);
		assets::AssetRegistry::get()->registerAssetType<assets::StaticMeshAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::TextureAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::SoundAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::ShaderAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::MaterialAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::MaterialInstanceAsset>();

		// Create Default Scene (if set)
		auto sceneData = sceneSubsystem->createScene(assets::AssetRegistry::get()->contentRoot() / mProjectFile.defaultScenePath);

		// Register Components to Scene Data Class
		sceneData->register_component<TransformComponent2D>();
		sceneData->register_component<TransformComponent3D>();
		sceneData->register_component<rendering::MeshComponent>();
		sceneData->register_component<rendering::LightComponent>();
		sceneData->register_component<rendering::ShadowCasterComponent>();
		sceneData->register_component<rendering::CameraComponent>();
		sceneData->register_component<scripting::AngelScriptComponent>();
		sceneData->register_component<rendering::ProceduralMeshComponent>();
		sceneData->register_component<procedural::PlaneComponent>();
		sceneData->register_component<procedural::TerrainComponent>();
		sceneData->register_component<procedural::IcoSphereComponent>();
		sceneData->register_component<physics::RigidbodyComponent2D>();
		sceneData->register_component<physics::BoxComponent2D>();
		sceneData->register_component<physics::CircleComponent2D>();
		sceneData->register_component<physics::RigidbodyComponent3D>();
		sceneData->register_component<physics::BoxComponent3D>();
		sceneData->register_component<physics::SphereComponent3D>();

		// Load Project Settings
		LoadSettings(assets::AssetRegistry::get()->projectRoot() / "config" / "Settings.json", mSettings);

		// Load/Initialize Assets
		assets::AssetRegistry::get()->loadAssetCache();
		//addDefaultAssets();
		//reimportDefaultAssets();
		//assets::AssetRegistry::get()->saveAssetCache();
		//loadAndResaveAssets();
	}

	void Engine::startup()
	{
		mRunning = true;
		mPlayState = PlayState::Stopped;

		// Initialize Systems
		{
			executeCallbacks(ExecutionStage::Startup);
		}

		mLastTime = glfwGetTime(); // Time Count Started
		mCurrentTime = mLastTime;

		if (constexpr bool setupDefaultScene = false; setupDefaultScene)
		{
			auto entt_subsystem = getSystem<ecs::EnTTSubsystem>();
			auto scene_graph = getSystem<scene::SceneGraph>();
			auto scene_subsystem = getSystem<io::SceneSubsystem>();
			auto sceneData = scene_subsystem->sceneData();

			// Create Default Scene in code -- used when scene serialization is changed
			defaultScene();
			//physicsScene2D();
			//physicsScene3D();
			//proceduralScene();

			sceneData->update_data(entt_subsystem, scene_graph);
			sceneData->save();
			sceneData->clear();
		}
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

		const auto audioSubsystem = getSystem<audio::AudioSubsystem>();

		// Update all Subsystems
		{
			executeCallbacks(ExecutionStage::SubsystemUpdate, true);
		}

		/*const auto inputSubsystem = getSystem<input::InputSubsystem>();
		if (inputSubsystem->justPressed("Play"))
		{
			play();
		}

		if (inputSubsystem->justPressed("Restart"))
		{
			restart();
		}*/

		// Call system start functions to prepare for gameplay
		if (mPlayState == PlayState::Started)
		{
			executeCallbacks(ExecutionStage::BeginPlay);

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::JustPaused)
		{
			//audioSubsystem->pauseAllSounds();

			mPlayState = PlayState::Paused;
		}

		if (mPlayState == PlayState::JustUnpaused)
		{
			//audioSubsystem->playAllSounds();

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

			//audioSubsystem->stopAllSounds();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		if (const auto windowSubsystem = getSystem<window::WindowSubsystem>(); windowSubsystem->shouldPrimaryWindowClose())
		{
			mRunning = false;
		}

		return mRunning;
	}

	void Engine::shutdown()
	{
		// Cleanup All Systems
		executeCallbacks(ExecutionStage::Shutdown);

		mSystems.clear();

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

		const fs::path& texturePath1 = "textures\\cube.ptexture";
		const fs::path& texturePath2 = "textures\\chalet.ptexture";
		const fs::path& texturePath3 = "textures\\space_engineer.ptexture";
		const fs::path& texturePath4 = "textures\\statue.ptexture";
		const fs::path& texturePath5 = "textures\\xsprite.ptexture";

		PuffinID textureId1 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath1)->id();
		PuffinID textureId2 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath2)->id();
		PuffinID textureId3 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath3)->id();
		PuffinID textureId4 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath4)->id();
		PuffinID textureId5 = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(texturePath5)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->addAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path shaderPath1 = "materials\\forward_shading\\forward_shading_vert.pshader";
		const fs::path shaderPath2 = "materials\\forward_shading\\forward_shading_frag.pshader";

		const auto shaderAsset1 = assets::AssetRegistry::get()->addAsset<assets::ShaderAsset>(shaderPath1);
		const auto shaderAsset2 = assets::AssetRegistry::get()->addAsset<assets::ShaderAsset>(shaderPath2);

		//shaderAsset1->setType(assets::ShaderType::Vertex);

		//shaderAsset1->setShaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.vert)"));
		//shaderAsset1->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_vs.spv)"));
		//shaderAsset1->loadCodeFromBinary();
		//shaderAsset1->save();

		//shaderAsset2->setType(assets::ShaderType::Fragment);
		//shaderAsset2->setShaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.frag)"));
		//shaderAsset2->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_fs.spv)"));
		//shaderAsset2->loadCodeFromBinary();
		//shaderAsset2->save();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

		const auto materialInstAsset1 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath1);
		const auto materialInstAsset2 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath2);

		materialInstAsset1->getTexIDs()[0] = textureId1;

		materialInstAsset1->save();

		materialInstAsset2->getTexIDs()[0] = textureId2;

		materialInstAsset2->save();
	}

	void Engine::reimportDefaultAssets()
	{
		io::load_and_import_model(R"(C:\Projects\PuffinProject\model_backups\cube.obj)", "meshes");
		io::load_and_import_model(R"(C:\Projects\PuffinProject\model_backups\space_engineer.obj)", "meshes");
		//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
		io::load_and_import_model(R"(C:\Projects\PuffinProject\model_backups\chalet.obj)", "meshes");

		io::load_and_import_texture(R"(C:\Projects\PuffinProject\texture_backups\chalet.jpg)", "textures");
		io::load_and_import_texture(R"(C:\Projects\PuffinProject\texture_backups\cube.png)", "textures");
		io::load_and_import_texture(R"(C:\Projects\PuffinProject\texture_backups\space_engineer.jpg)", "textures");
		io::load_and_import_texture(R"(C:\Projects\PuffinProject\texture_backups\statue.jpg)", "textures");
		io::load_and_import_texture(R"(C:\Projects\PuffinProject\texture_backups\xsprite.png)", "textures");
	}

	void Engine::loadAndResaveAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";
		const fs::path& texturePath3 = "textures\\space_engineer.ptexture";
		const fs::path& texturePath4 = "textures\\statue.ptexture";
		const fs::path& texturePath5 = "textures\\xsprite.ptexture";

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		const fs::path shaderPath1 = "materials\\forward_shading\\forward_shading_vert.pshader";
		const fs::path shaderPath2 = "materials\\forward_shading\\forward_shading_frag.pshader";

		const fs::path materialInstPath1 = "materials\\forward_shading\\forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = "materials\\forward_shading\\forward_shading_chalet.pmaterialinst";

		std::vector paths =
		{
			meshPath1, meshPath2, meshPath3, meshPath4,
			texturePath1, texturePath2, texturePath3, texturePath4, texturePath5,
			shaderPath1, shaderPath2,
			materialInstPath1, materialInstPath2
		};

		for (const auto path : paths)
		{
			if (const auto asset = assets::AssetRegistry::get()->getAsset(path); asset != nullptr)
			{
				asset->load();
				asset->save();
				asset->unload();
			}
		}
	}

	void Engine::defaultScene()
	{
		// Get assets
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

		const PuffinID textureId1 = assets::AssetRegistry::get()->getAsset<assets::TextureAsset>(texturePath1)->id();
		const PuffinID textureId2 = assets::AssetRegistry::get()->getAsset<assets::TextureAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath1)->id();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath2)->id();

		const auto scene_graph = getSystem<scene::SceneGraph>();

		auto& house_node = scene_graph->add_node<rendering::MeshNode>();
		house_node.set_name("House");
		house_node.set_position({ 2.0f, 0.0f, 0.0f });
		house_node.set_mesh_asset_id(meshId1);
		house_node.set_mat_asset_id(materialInstId1);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.set_name("Sphere");
		sphere.set_position({ -1.0f, -0.0f, 0.0f });
		sphere.set_mesh_asset_id(meshId2);
		sphere.set_mat_asset_id(materialInstId1);*/

		auto& cube_1 = scene_graph->add_node<rendering::MeshNode>();
		cube_1.name() = "Cube_1";
		cube_1.set_position({ 0.0f });
		cube_1.set_mesh_asset_id(meshId3);
		cube_1.set_mat_asset_id(materialInstId1);

		auto& cube_2 = scene_graph->add_node<rendering::MeshNode>();
		cube_2.set_name("Cube_2");
		cube_2.set_position({ -1.75f, -5.0f, 0.0f });
		cube_2.set_mesh_asset_id(meshId3);
		cube_2.set_mat_asset_id(materialInstId1);

		auto& plane = scene_graph->add_node<rendering::MeshNode>();
		plane.set_name("Plane");
		plane.set_position({ 0.0f, -10.0f, 0.0f });
		plane.set_scale({ 50.0f, 1.0f, 50.0f });
		plane.set_mesh_asset_id(meshId3);
		plane.set_mat_asset_id(materialInstId1);

		auto& dir_light = scene_graph->add_node<rendering::LightNode3D>();
		dir_light.set_name("Directional Light");
		dir_light.set_position({ 0.0f, 10.0f, 0.0f });
		dir_light.set_color({ .01f });
		dir_light.set_light_type(rendering::LightType::Directional);
		dir_light.set_ambient_intensity(0.f);

		auto& dir_light_mesh = scene_graph->add_child_node<rendering::MeshNode>(dir_light.id());
		dir_light_mesh.set_scale({ 0.25f });
		dir_light_mesh.set_mesh_asset_id(meshId3);
		dir_light_mesh.set_mat_asset_id(materialInstId1);

		auto& spot_light = scene_graph->add_node<rendering::LightNode3D>();
		spot_light.set_name("Spot Light");
		spot_light.set_position({ 10.0f, 5.0f, 0.0f });
		spot_light.set_light_type(rendering::LightType::Spot);
		spot_light.set_direction({ -0.5f, -0.5f, 0.f });
		spot_light.set_ambient_intensity(0.f);

		auto& spot_light_mesh = scene_graph->add_child_node<rendering::MeshNode>(spot_light.id());
		spot_light_mesh.set_scale({ 0.25f });
		spot_light_mesh.set_mesh_asset_id(meshId3);
		spot_light_mesh.set_mat_asset_id(materialInstId1);

		//auto& shadow = registry->emplace<rendering::ShadowCasterComponent>(entities[6]);
		//shadow.width = 4096;
		//shadow.height = 4096;

		//auto& script = registry->emplace<scripting::AngelScriptComponent>(entities[0]);
		//script.name = "ExampleScript";
		//script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::physicsScene3D()
	{
		// Get assets
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

		const PuffinID textureId1 = assets::AssetRegistry::get()->getAsset<assets::TextureAsset>(texturePath1)->id();
		const PuffinID textureId2 = assets::AssetRegistry::get()->getAsset<assets::TextureAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath1)->id();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->addAsset<assets::MaterialInstanceAsset>(materialInstPath2)->id();

		const auto scene_graph = getSystem<scene::SceneGraph>();

		// Light node

		auto& light = scene_graph->add_node<rendering::LightNode3D>();
		light.position().y = 50.0f;
		light.set_light_type(rendering::LightType::Directional);
		light.set_ambient_intensity(0.01f);

		constexpr float floor_width = 2000.0f;

		std::vector<float> y_offsets;
		for (int i = 0; i < 10; ++i)
		{
			y_offsets.push_back(i * 10.0f + 10.0f);
		}

		// Floor node
		//{
		//	const auto floorEntity = enttSubsystem->createEntity("Floor");

		//	auto& transform = registry->emplace<TransformComponent3D>(floorEntity, Vector3d(0.0f), glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f)), Vector3f(floorWidth, 1.0f, floorWidth));

		//	registry->emplace<rendering::MeshComponent>(floorEntity, meshId3, materialInstId1);

		//	registry->emplace<physics::BoxComponent3D>(floorEntity, Vector3f(floorWidth, 1.0f, floorWidth));

		//	registry->emplace<physics::RigidbodyComponent3D>(floorEntity);
		//}

		auto& floor_body = scene_graph->add_node<physics::RigidbodyNode3D>();

		//// Create Box Entities
		//{
		//	constexpr int numBodiesX = 100;
		//	constexpr int numBodiesZ = 100;
		//	constexpr int numBodies = numBodiesX * numBodiesZ;

		//	constexpr float xStartPosition = floorWidth - 10.0f;
		//	constexpr float zStartPosition = floorWidth - 10.0f;

		//	const Vector3d startPosition(-xStartPosition, 0.f, -zStartPosition);
		//	const Vector3d endPosition(xStartPosition, 0.f, zStartPosition);

		//	Vector3f positionOffset = endPosition - startPosition;
		//	positionOffset.x /= numBodiesX;
		//	positionOffset.z /= numBodiesZ;

		//	int i = 0;
		//	for (int x = 0; x < numBodiesX; x++)
		//	{
		//		for (int z = 0; z < numBodiesZ; z++)
		//		{
		//			const std::string name = "Box";
		//			const auto boxEntity = enttSubsystem->createEntity(name);

		//			Vector3f position = startPosition;
		//			position.x += positionOffset.x * static_cast<float>(x);
		//			position.z += positionOffset.z * static_cast<float>(z);

		//			const int yIdx = i % yOffsets.size();
		//			position.y = yOffsets[yIdx];

		//			registry->emplace<TransformComponent3D>(boxEntity, position);

		//			registry->emplace<rendering::MeshComponent>(boxEntity, meshId3, materialInstId1);

		//			registry->emplace<physics::BoxComponent3D>(boxEntity, Vector3f(1.0f));

		//			registry->emplace<physics::RigidbodyComponent3D>(boxEntity, physics::BodyType::Dynamic, 1.0f);

		//			i++;
		//		}
		//	}
		//}
	}

	void Engine::proceduralScene()
	{
		//auto ecsWorld = getSystem<ECS::World>();

		//// Initialize Assets
		//fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		//const fs::path& cubeMeshPath = "meshes\\cube.pstaticmesh";

		//PuffinId cubeMeshId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeMeshPath)->id();

		//const fs::path& cubeTexturePath = "textures\\cube.ptexture";

		//PuffinId cubeTextureId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeTexturePath)->id();

		//const auto lightEntity = ECS::CreateEntity(ecsWorld);
		//lightEntity->SetName("Light");
		//lightEntity->AddComponent<TransformComponent3D>();
		//lightEntity->GetComponent<TransformComponent3D>().position = { 0.0, 10.0, 0.0 };
		//lightEntity->GetComponent<TransformComponent3D>().scale = { 0.25f };
		//lightEntity->AddComponent<rendering::LightComponent>();
		//lightEntity->GetComponent<rendering::LightComponent>().type = rendering::LightType::Directional;
		//lightEntity->AddComponent<rendering::MeshComponent>();
		//lightEntity->GetComponent<rendering::MeshComponent>().meshAssetID = cubeMeshId;
		//lightEntity->GetComponent<rendering::MeshComponent>().textureAssetId = cubeTextureId;
		////lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		//const auto planeEntity = ECS::CreateEntity(ecsWorld);
		//planeEntity->SetName("Terrain");
		//planeEntity->AddAndGetComponent<TransformComponent3D>().position = { 0.0, -10.0f, 0.0 };
		//planeEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		//planeEntity->AddComponent<procedural::TerrainComponent>();
		//planeEntity->GetComponent<procedural::TerrainComponent>().halfSize = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().numQuads = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().heightMultiplier = 10;

		//const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		//sphereEntity->SetName("Sphere");
		//sphereEntity->AddAndGetComponent<TransformComponent3D>().position = { 0.0, 5.0, 0.0 };
		//sphereEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		//sphereEntity->AddComponent<procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent3D>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;*/
	}

	void Engine::play()
	{
        if (mPlayState == PlayState::Stopped)
        {
            mPlayState = PlayState::Started;
        }
		else if (mPlayState == PlayState::Playing)
        {
	        mPlayState = PlayState::JustPaused;
        }
        else if (mPlayState == PlayState::Paused)
        {
	        mPlayState = PlayState::JustUnpaused;
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
