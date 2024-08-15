#include "puffin/core/engine.h"

#include <chrono>
#include <thread>

#include "puffin/assets/assetimporters.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/materialasset.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/assets/shaderasset.h"
#include "puffin/assets/soundasset.h"
#include "puffin/assets/textureasset.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/shapecomponents2d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/shapecomponents3d.h"
#include "puffin/components/procedural/proceduralmeshcomponent.h"
#include "puffin/components/rendering/cameracomponent.h"
#include "puffin/components/rendering/lightcomponent.h"
#include "puffin/components/rendering/meshcomponent.h"
#include "puffin/components/scripting/angelscriptcomponent.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/nodes/physics/rigidbodynode3d.h"
#include "puffin/nodes/rendering/lightnode3d.h"
#include "puffin/nodes/rendering/meshnode.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/scene/scenegraph.h"
#include "puffin/scene/scenesubsystem.h"
#include "puffin/editor/ui/editoruisubsystem.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/subsystemmanager.h"
#include "puffin/utility/performancebenchmarksubsystem.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace puffin
{
	void AddDefaultEngineArguments(argparse::ArgumentParser& parser)
	{
		parser.add_argument("-p", "-project", "-project_path")
		      .help("Specify the path of the project file")
		      .required();

		parser.add_argument("-s", "-scene")
		      .help("Specify the scene file to load on launch")
		      .default_value("");

		parser.add_argument("--setup-engine-default-scene")
		      .help("Specify whether the engine default scene should be initialized on launch")
		      .default_value(false)
		      .implicit_value(true);

		parser.add_argument("--setup-default-settings")
		      .help("Specify whether to setup settings file with engine default settings")
		      .default_value(false)
		      .implicit_value(true);
	}
}

namespace puffin::core
{
	Engine::Engine()
	{
	}

	Engine::~Engine()
	{
		mApplication = nullptr;
		mSubsystemManager = nullptr;
	}

	void Engine::Setup()
	{
		mSubsystemManager = std::make_unique<SubsystemManager>(shared_from_this());

		RegisterRequiredSubsystems();
	}

	void Engine::Initialize(const argparse::ArgumentParser& parser)
	{
		fs::path projectPath = fs::path(parser.get<std::string>("-project_path")).make_preferred();

		// Load Project File
		load_project(projectPath, mProjectFile);

		// Setup asset registry
		assets::AssetRegistry::get()->init(mProjectFile, projectPath);
		assets::AssetRegistry::get()->register_asset_type<assets::StaticMeshAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::TextureAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::SoundAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::ShaderAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::MaterialAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::MaterialInstanceAsset>();

		// Load Project Settings
		mSetupEngineDefaultSettings = parser.get<bool>("--setup-default-settings");
		mSetupEngineDefaultScene = parser.get<bool>("--setup-engine-default-scene");

		// Load/Initialize Assets
		assets::AssetRegistry::get()->load_asset_cache();
		//add_default_assets();
		//reimport_default_assets();
		//assets::AssetRegistry::get()->save_asset_cache();
		//load_and_resave_assets();

		// Initialize engine subsystems
		mSubsystemManager->CreateAndInitializeEngineSubsystems();

		// Register components to scene subsystem
		auto sceneSubsystem = GetSubsystem<io::SceneSubsystem>();

		sceneSubsystem->register_component<TransformComponent2D>();
		sceneSubsystem->register_component<TransformComponent3D>();
		sceneSubsystem->register_component<rendering::MeshComponent>();
		sceneSubsystem->register_component<rendering::LightComponent>();
		sceneSubsystem->register_component<rendering::ShadowCasterComponent>();
		sceneSubsystem->register_component<rendering::CameraComponent3D>();
		sceneSubsystem->register_component<scripting::AngelScriptComponent>();
		sceneSubsystem->register_component<rendering::ProceduralMeshComponent>();
		sceneSubsystem->register_component<procedural::PlaneComponent>();
		sceneSubsystem->register_component<procedural::TerrainComponent>();
		sceneSubsystem->register_component<procedural::IcoSphereComponent>();
		sceneSubsystem->register_component<physics::RigidbodyComponent2D>();
		sceneSubsystem->register_component<physics::BoxComponent2D>();
		sceneSubsystem->register_component<physics::CircleComponent2D>();
		sceneSubsystem->register_component<physics::RigidbodyComponent3D>();
		sceneSubsystem->register_component<physics::BoxComponent3D>();
		sceneSubsystem->register_component<physics::SphereComponent3D>();

		// Load default scene
		auto sceneString = parser.get<std::string>("-scene");
		if (!sceneString.empty())
		{
			mLoadSceneOnLaunch = true;
		}

		{
			auto sceneData = sceneSubsystem->create_scene(
				assets::AssetRegistry::get()->content_root() / sceneString);

			if (mSetupEngineDefaultScene)
			{
				auto entt_subsystem = GetSubsystem<ecs::EnTTSubsystem>();
				auto scene_graph = GetSubsystem<scene::SceneGraphSubsystem>();

				// Create Default Scene in code -- used when scene serialization is changed
				InitDefaultScene();
				//physicsScene2D();
				//physicsScene3D();
				//proceduralScene();

				sceneData->update_data(entt_subsystem, scene_graph);
				sceneData->save();
			}
			else if (mLoadSceneOnLaunch)
			{
				sceneData->load();
			}
		}

		sceneSubsystem->setup();

		mLastTime = glfwGetTime(); // Time Count Started
		mCurrentTime = mLastTime;

		mRunning = true;
		mPlayState = PlayState::Stopped;
	}

	bool Engine::Update()
	{
		// Run Game Loop;

		// Process input
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->start_benchmark("Input");

			auto inputSubsystem = mSubsystemManager->GetInputSubsystem();
			inputSubsystem->ProcessInput();

			benchmarkSubsystem->end_benchmark("Input");
		}

		// Wait for last presentation to complete and sample delta time
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->start_benchmark("Sample Time");

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			double sampledTime = renderSubsystem->WaitForLastPresentationAndSampleTime();
			UpdateDeltaTime(sampledTime);

			benchmarkSubsystem->end_benchmark("Sample Time");
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (mDeltaTime > mTimeStepLimit)
		{
			mDeltaTime = mTimeStepLimit;
		}

		const auto AudioSubsystem = GetSubsystem<audio::AudioSubsystem>();

		// Execute engine updates
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->start_benchmark("Engine Update");

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				if (subsystem->ShouldUpdate())
				{
					benchmarkSubsystem->start_benchmark_category(subsystem->GetName(), "Engine Update");

					subsystem->Update(mDeltaTime);

					benchmarkSubsystem->end_benchmark_category(subsystem->GetName(), "Engine Update");
				}
			}

			benchmarkSubsystem->end_benchmark("Engine Update");
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
		if (mPlayState == PlayState::BeginPlay)
		{
			mSubsystemManager->CreateAndInitializeGameplaySubsystems();

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				subsystem->BeginPlay();
			}

			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->BeginPlay();
			}

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
				auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmarkSubsystem->start_benchmark("Fixed Update");

				// Add onto accumulated time
				mAccumulatedTime += mDeltaTime;

				while (mAccumulatedTime >= mTimeStepFixed)
				{
					mAccumulatedTime -= mTimeStepFixed;

					for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
					{
						if (subsystem->ShouldFixedUpdate())
						{
							benchmarkSubsystem->start_benchmark_category(subsystem->GetName(), "Fixed Update");

							subsystem->FixedUpdate(mTimeStepFixed);

							benchmarkSubsystem->end_benchmark_category(subsystem->GetName(), "Fixed Update");
						}
					}
				}

				benchmarkSubsystem->end_benchmark("Fixed Update");
			}

			// Update
			{
				auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmarkSubsystem->start_benchmark("Update");

				for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
				{
					if (subsystem->ShouldUpdate())
					{
						benchmarkSubsystem->start_benchmark_category(subsystem->GetName(), "Update");

						subsystem->Update(mDeltaTime);

						benchmarkSubsystem->end_benchmark_category(subsystem->GetName(), "Update");
					}
				}

				benchmarkSubsystem->end_benchmark("Update");
			}
		}

		// Render
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->start_benchmark("Render");

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			renderSubsystem->Render(mDeltaTime);

			benchmarkSubsystem->end_benchmark("Render");
		}

		if (mPlayState == PlayState::EndPlay)
		{
			// End play and cleanup gameplay subsystems
			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();

			//audioSubsystem->stopAllSounds();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		if (const auto windowSubsystem = GetSubsystem<window::WindowSubsystem>(); windowSubsystem->
			should_primary_window_close())
		{
			mRunning = false;
		}

		return mRunning;
	}

	void Engine::Deinitialize()
	{
		// Cleanup any running gameplay subsystems
		if (mPlayState == PlayState::Playing)
		{
			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();
		}

		// Cleanup all engine subsystems
		mSubsystemManager->DestroyEngineSubsystems();

		// Clear Asset Registry
		assets::AssetRegistry::clear();

		mSubsystemManager = nullptr;
	}

	void Engine::RegisterRequiredSubsystems() const
	{
		RegisterSubsystem<window::WindowSubsystem>();
		RegisterSubsystem<core::SignalSubsystem>();
		RegisterSubsystem<input::InputSubsystem>();
		RegisterSubsystem<utility::PerformanceBenchmarkSubsystem>();
		RegisterSubsystem<SettingsManager>();
		RegisterSubsystem<EnkiTSSubsystem>();
		RegisterSubsystem<audio::AudioSubsystem>();
		RegisterSubsystem<ecs::EnTTSubsystem>();
		RegisterSubsystem<scene::SceneGraphSubsystem>();
		RegisterSubsystem<io::SceneSubsystem>();
		RegisterSubsystem<rendering::CameraSubystem>();

		RegisterSubsystem<ui::EditorUISubsystem>();

		RegisterSubsystem<scene::SceneGraphGameplaySubsystem>();
	}

	void Engine::AddDefaultAssets()
	{
		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		PuffinID meshId1 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath1)->GetID();
		//PuffinID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		PuffinID meshId3 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath3)->GetID();
		PuffinID meshId4 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = fs::path() / "textures" / "cube.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath3 = fs::path() / "textures" / "space_engineer.ptexture";
		const fs::path& texturePath4 = fs::path() / "textures" / "statue.ptexture";
		const fs::path& texturePath5 = fs::path() / "textures" / "xsprite.ptexture";

		PuffinID textureId1 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath1)->GetID();
		PuffinID textureId2 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath2)->GetID();
		PuffinID textureId3 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath3)->GetID();
		PuffinID textureId4 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath4)->GetID();
		PuffinID textureId5 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath5)->GetID();

		const fs::path& soundPath1 = fs::path() / "sounds" / "Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->add_asset<assets::SoundAsset>(soundPath1)->GetID();

		const fs::path shaderPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_vert.pshader";
		const fs::path shaderPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_frag.pshader";

		const auto shaderAsset1 = assets::AssetRegistry::get()->add_asset<assets::ShaderAsset>(shaderPath1);
		const auto shaderAsset2 = assets::AssetRegistry::get()->add_asset<assets::ShaderAsset>(shaderPath2);

		//shaderAsset1->setType(assets::ShaderType::Vertex);

		//shaderAsset1->setshaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.vert)"));
		//shaderAsset1->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_vs.spv)"));
		//shaderAsset1->loadCodeFromBinary();
		//shaderAsset1->save();

		//shaderAsset2->setType(assets::ShaderType::Fragment);
		//shaderAsset2->setshaderPath(fs::path(R"(C:\Projects\PuffinEngine\shaders\vulkan\forward_shading\forward_shading.frag)"));
		//shaderAsset2->setBinaryPath(fs::path(R"(C:\Projects\PuffinEngine\bin\vulkan\forward_shading\forward_shading_fs.spv)"));
		//shaderAsset2->loadCodeFromBinary();
		//shaderAsset2->save();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		const auto materialInstAsset1 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(
			materialInstPath1);
		const auto materialInstAsset2 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(
			materialInstPath2);

		materialInstAsset1->getTexIDs()[0] = textureId1;

		materialInstAsset1->Save();

		materialInstAsset2->getTexIDs()[0] = textureId2;

		materialInstAsset2->Save();
	}

	void Engine::ReimportDefaultAssets()
	{
		io::LoadAndImportModel(assets::AssetRegistry::get()->project_root() / "model_backups/cube.obj", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::get()->project_root() / "model_backups/space_engineer.obj",
		                          "meshes");
		//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::get()->project_root() / "model_backups/chalet.obj", "meshes");

		io::LoadAndImportTexture(assets::AssetRegistry::get()->project_root() / "texture_backups/chalet.jpg",
		                            "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::get()->project_root() / "texture_backups/cube.png",
		                            "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::get()->project_root() / "texture_backups/space_engineer.jpg",
		                            "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::get()->project_root() / "texture_backups/statue.jpg",
		                            "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::get()->project_root() / "texture_backups/xsprite.png",
		                            "textures");
	}

	void Engine::LoadAndResaveAssets()
	{
		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		const fs::path& texturePath1 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "cube.ptexture";
		const fs::path& texturePath3 = fs::path() / "textures" / "space_engineer.ptexture";
		const fs::path& texturePath4 = fs::path() / "textures" / "statue.ptexture";
		const fs::path& texturePath5 = fs::path() / "textures" / "xsprite.ptexture";

		const fs::path& soundPath1 = fs::path() / "sounds" / "Select 1.wav";

		const fs::path shaderPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_vert.pshader";
		const fs::path shaderPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_frag.pshader";

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		std::vector paths =
		{
			meshPath1, meshPath2, meshPath3, meshPath4,
			texturePath1, texturePath2, texturePath3, texturePath4, texturePath5,
			shaderPath1, shaderPath2,
			materialInstPath1, materialInstPath2
		};

		for (const auto path : paths)
		{
			if (const auto asset = assets::AssetRegistry::get()->get_asset(path); asset != nullptr)
			{
				asset->Load();
				asset->Save();
				asset->Unload();
			}
		}
	}

	void Engine::InitDefaultScene()
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::get()->content_root();

		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath1)->GetID();
		//const PuffinID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		const PuffinID meshId3 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath3)->GetID();
		const PuffinID meshId4 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "cube.ptexture";

		const PuffinID textureId1 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath1)->GetID();
		const PuffinID textureId2 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath2)->GetID();

		//const fs::path& soundPath1 = "sounds/Select 1.wav";

		//PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->get_asset<assets::MaterialInstanceAsset>(
			materialInstPath1)->GetID();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->get_asset<assets::MaterialInstanceAsset>(
			materialInstPath2)->GetID();

		auto registry = GetSubsystem<ecs::EnTTSubsystem>()->registry();
		const auto sceneGraph = GetSubsystem<scene::SceneGraphSubsystem>();

		auto houseNode = sceneGraph->add_node<rendering::MeshNode>();
		houseNode->set_name("House");
		houseNode->set_position({2.0f, 0.0f, 0.0f});
		houseNode->set_mesh_asset_id(meshId1);
		houseNode->set_mat_asset_id(materialInstId1);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.set_name("Sphere");
		sphere.set_position({ -1.0f, -0.0f, 0.0f });
		sphere.set_mesh_asset_id(meshId2);
		sphere.set_mat_asset_id(materialInstId1);*/

		auto cube1 = sceneGraph->add_node<rendering::MeshNode>();
		cube1->name() = "Cube_1";
		cube1->set_position({0.0f});
		cube1->set_mesh_asset_id(meshId3);
		cube1->set_mat_asset_id(materialInstId1);

		auto cube2 = sceneGraph->add_node<rendering::MeshNode>();
		cube2->set_name("Cube_2");
		cube2->set_position({-1.75f, -5.0f, 0.0f});
		cube2->set_mesh_asset_id(meshId3);
		cube2->set_mat_asset_id(materialInstId1);

		auto plane = sceneGraph->add_node<rendering::MeshNode>();
		plane->set_name("Plane");
		plane->set_position({0.0f, -10.0f, 0.0f});
		plane->set_scale({50.0f, 1.0f, 50.0f});
		plane->set_mesh_asset_id(meshId3);
		plane->set_mat_asset_id(materialInstId1);

		auto dirLight = sceneGraph->add_node<rendering::LightNode3D>();
		dirLight->set_name("Directional Light");
		dirLight->set_position({0.0f, 10.0f, 0.0f});
		dirLight->set_color({.05f});
		dirLight->set_ambient_intensity(.0f);
		dirLight->set_light_type(rendering::LightType::Directional);
		dirLight->set_ambient_intensity(0.f);
		dirLight->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(dirLight->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
			shadow.bias_min = 0.3f;
			shadow.bias_max = 0.5f;
		});

		UpdateTransformOrientation(*dirLight->transform_3d(), {0.0f, -90.0f, 0.0f});

		auto dirLightMesh = sceneGraph->add_child_node<rendering::MeshNode>(dirLight->id());
		dirLightMesh->set_scale({0.25f});
		dirLightMesh->set_mesh_asset_id(meshId3);
		dirLightMesh->set_mat_asset_id(materialInstId1);

		auto spotLight = sceneGraph->add_node<rendering::LightNode3D>();
		spotLight->set_name("Spot Light");
		spotLight->set_position({-10.0f, 5.0f, 0.0f});
		spotLight->set_light_type(rendering::LightType::Spot);
		spotLight->set_color({0.5f, 0.5f, 1.0f});
		spotLight->set_ambient_intensity(0.f);
		spotLight->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(spotLight->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		auto spotLightMesh = sceneGraph->add_child_node<rendering::MeshNode>(spotLight->id());
		spotLightMesh->set_scale({0.25f});
		spotLightMesh->set_mesh_asset_id(meshId3);
		spotLightMesh->set_mat_asset_id(materialInstId1);

		auto spotLight2 = sceneGraph->add_node<rendering::LightNode3D>();
		spotLight2->set_name("Spot Light 2");
		spotLight2->set_position({10.0f, 5.0f, 0.0f});
		spotLight2->set_light_type(rendering::LightType::Spot);
		spotLight2->set_color({1.0f, 0.5f, 0.5f});
		spotLight2->set_ambient_intensity(0.f);
		spotLight2->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(spotLight2->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		UpdateTransformOrientation(*spotLight2->transform_3d(), {0.0f, 180.0f, 0.0f});

		auto spotLightMesh2 = sceneGraph->add_child_node<rendering::MeshNode>(spotLight2->id());
		spotLightMesh2->set_scale({0.25f});
		spotLightMesh2->set_mesh_asset_id(meshId3);
		spotLightMesh2->set_mat_asset_id(materialInstId1);

		//auto& script = registry->emplace<scripting::AngelScriptComponent>(entities[0]);
		//script.name = "ExampleScript";
		//script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::InitPhysicsScene3D()
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::get()->content_root();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath1)->GetID();
		const PuffinID meshId2 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath2)->GetID();
		const PuffinID meshId3 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath3)->GetID();
		const PuffinID meshId4 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		const PuffinID textureId1 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath1)->GetID();
		const PuffinID textureId2 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath2)->GetID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->get_asset<assets::SoundAsset>(soundPath1)->GetID();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(
			materialInstPath1)->GetID();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(
			materialInstPath2)->GetID();

		const auto sceneGraph = GetSubsystem<scene::SceneGraphSubsystem>();

		// Light node

		auto light = sceneGraph->add_node<rendering::LightNode3D>();
		light->position().y = 50.0f;
		light->set_light_type(rendering::LightType::Directional);
		light->set_ambient_intensity(0.01f);

		constexpr float floorWidth = 2000.0f;

		std::vector<float> yOffsets;
		for (int i = 0; i < 10; ++i)
		{
			yOffsets.push_back(i * 10.0f + 10.0f);
		}

		// Floor node
		//{
		//	const auto floorEntity = enttSubsystem->createEntity("Floor");

		//	auto& transform = registry->emplace<TransformComponent3D>(floorEntity, Vector3d(0.0f), glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f)), Vector3f(floorWidth, 1.0f, floorWidth));

		//	registry->emplace<rendering::MeshComponent>(floorEntity, meshId3, materialInstId1);

		//	registry->emplace<physics::BoxComponent3D>(floorEntity, Vector3f(floorWidth, 1.0f, floorWidth));

		//	registry->emplace<physics::RigidbodyComponent3D>(floorEntity);
		//}

		auto floorBody = sceneGraph->add_node<physics::RigidbodyNode3D>();

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

	void Engine::InitProceduralScene()
	{
		//auto ecsWorld = getSystem<ECS::World>();

		//// Initialize Assets
		//fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		//const fs::path& cubemeshPath = "meshes\\cube.pstaticmesh";

		//PuffinId cubemeshId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubemeshPath)->id();

		//const fs::path& cubetexturePath = "textures\\cube.ptexture";

		//PuffinId cubetextureId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubetexturePath)->id();

		//const auto lightEntity = ECS::CreateEntity(ecsWorld);
		//lightEntity->SetName("Light");
		//lightEntity->AddComponent<TransformComponent3D>();
		//lightEntity->GetComponent<TransformComponent3D>().position = { 0.0, 10.0, 0.0 };
		//lightEntity->GetComponent<TransformComponent3D>().scale = { 0.25f };
		//lightEntity->AddComponent<rendering::LightComponent>();
		//lightEntity->GetComponent<rendering::LightComponent>().type = rendering::LightType::Directional;
		//lightEntity->AddComponent<rendering::MeshComponent>();
		//lightEntity->GetComponent<rendering::MeshComponent>().meshAssetID = cubemeshId;
		//lightEntity->GetComponent<rendering::MeshComponent>().textureAssetId = cubetextureId;
		////lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		//const auto planeEntity = ECS::CreateEntity(ecsWorld);
		//planeEntity->SetName("Terrain");
		//planeEntity->AddAndGetComponent<TransformComponent3D>().position = { 0.0, -10.0f, 0.0 };
		//planeEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubetextureId;
		//planeEntity->AddComponent<procedural::TerrainComponent>();
		//planeEntity->GetComponent<procedural::TerrainComponent>().halfSize = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().numQuads = { 50 };
		//planeEntity->GetComponent<procedural::TerrainComponent>().heightMultiplier = 10;

		//const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		//sphereEntity->SetName("Sphere");
		//sphereEntity->AddAndGetComponent<TransformComponent3D>().position = { 0.0, 5.0, 0.0 };
		//sphereEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubetextureId;
		//sphereEntity->AddComponent<procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent3D>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubemeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubetextureId;*/
	}

	void Engine::Play()
	{
		if (mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::BeginPlay;
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

	void Engine::Restart()
	{
		if (mPlayState == PlayState::Playing || mPlayState == PlayState::Paused || mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::EndPlay;
		}
	}

	void Engine::Exit()
	{
		mRunning = false;
	}

	void Engine::UpdateDeltaTime(double sampledTime)
	{
		mLastTime = mCurrentTime;
		mCurrentTime = sampledTime;
		mDeltaTime = mCurrentTime - mLastTime;

		if (mFrameRateMax > 0)
		{
			const double deltaTimeMax = 1.0 / mFrameRateMax;
			double idleStartTime = 0.0;

			while (mDeltaTime < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				mCurrentTime = glfwGetTime();
				mDeltaTime = mCurrentTime - mLastTime;
			}
		}
	}
}
