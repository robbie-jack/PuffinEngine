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
    void add_default_engine_arguments(argparse::ArgumentParser &parser)
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
		m_application = nullptr;
		m_subsystem_manager = nullptr;
	}

	void Engine::setup()
	{
		m_subsystem_manager = std::make_unique<SubsystemManager>(shared_from_this());

		register_required_subsystems();
	}

	void Engine::initialize(const argparse::ArgumentParser& parser)
	{
		fs::path project_path = fs::path(parser.get<std::string>("-project_path")).make_preferred();

		// Load Project File
		load_project(project_path, m_project_file);

		// Setup asset registry
		assets::AssetRegistry::get()->init(m_project_file, project_path);
		assets::AssetRegistry::get()->register_asset_type<assets::StaticMeshAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::TextureAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::SoundAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::ShaderAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::MaterialAsset>();
		assets::AssetRegistry::get()->register_asset_type<assets::MaterialInstanceAsset>();

		// Load Project Settings
		m_setup_engine_default_settings = parser.get<bool>("--setup-default-settings");
		m_setup_engine_default_scene = parser.get<bool>("--setup-engine-default-scene");

		// Load/Initialize Assets
		assets::AssetRegistry::get()->load_asset_cache();
		//add_default_assets();
		//reimport_default_assets();
		//assets::AssetRegistry::get()->save_asset_cache();
		//load_and_resave_assets();

		// Initialize engine subsystems
		m_subsystem_manager->create_and_initialize_engine_subsystems();

		// Register components to scene subsystem
		auto scene_subsystem = get_subsystem<io::SceneSubsystem>();

		scene_subsystem->register_component<TransformComponent2D>();
		scene_subsystem->register_component<TransformComponent3D>();
		scene_subsystem->register_component<rendering::MeshComponent>();
		scene_subsystem->register_component<rendering::LightComponent>();
		scene_subsystem->register_component<rendering::ShadowCasterComponent>();
		scene_subsystem->register_component<rendering::CameraComponent3D>();
		scene_subsystem->register_component<scripting::AngelScriptComponent>();
		scene_subsystem->register_component<rendering::ProceduralMeshComponent>();
		scene_subsystem->register_component<procedural::PlaneComponent>();
		scene_subsystem->register_component<procedural::TerrainComponent>();
		scene_subsystem->register_component<procedural::IcoSphereComponent>();
		scene_subsystem->register_component<physics::RigidbodyComponent2D>();
		scene_subsystem->register_component<physics::BoxComponent2D>();
		scene_subsystem->register_component<physics::CircleComponent2D>();
		scene_subsystem->register_component<physics::RigidbodyComponent3D>();
		scene_subsystem->register_component<physics::BoxComponent3D>();
		scene_subsystem->register_component<physics::SphereComponent3D>();

		// Load default scene
		auto scene_string = parser.get<std::string>("-scene");
		if (!scene_string.empty())
		{
			m_load_scene_on_launch = true;
		}

		auto scene_data = scene_subsystem->create_scene(
			assets::AssetRegistry::get()->content_root() / scene_string);

		{
            if (m_setup_engine_default_scene)
            {
                auto entt_subsystem = get_subsystem<ecs::EnTTSubsystem>();
                auto scene_graph = get_subsystem<scene::SceneGraphSubsystem>();

                // Create Default Scene in code -- used when scene serialization is changed
                default_scene();
                //physicsScene2D();
                //physicsScene3D();
                //proceduralScene();

                scene_data->update_data(entt_subsystem, scene_graph);
                scene_data->save();
            }
			else if (m_load_scene_on_launch)
            {
                scene_data->load();
            }
		}

		scene_subsystem->setup();

		m_last_time = glfwGetTime(); // Time Count Started
		m_current_time = m_last_time;

		m_running = true;
		m_play_state = PlayState::Stopped;
	}

	bool Engine::update()
	{
		// Run Game Loop;

		// Process input
		{
			auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmark_subsystem->start_benchmark("Input");

			auto input_subsystem = m_subsystem_manager->get_input_subsystem();
			input_subsystem->process_input();

			benchmark_subsystem->end_benchmark("Input");
		}

		// Wait for last presentation to complete and sample delta time
		{
			auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmark_subsystem->start_benchmark("Sample Time");

			auto render_subsystem = m_subsystem_manager->get_render_subsystem();

			double sampled_time = render_subsystem->wait_for_last_presentation_and_sample_time();
			update_delta_time(sampled_time);

			benchmark_subsystem->end_benchmark("Sample Time");
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (m_delta_time > m_time_step_limit)
		{
			m_delta_time = m_time_step_limit;
		}

		const auto audio_subsystem = get_subsystem<audio::AudioSubsystem>();

		// Execute engine updates
		{
			auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmark_subsystem->start_benchmark("Engine Update");

			for (auto subsystem : m_subsystem_manager->get_subsystems())
			{
				if (subsystem->should_update())
				{
					benchmark_subsystem->start_benchmark_category(subsystem->name(), "Engine Update");

					subsystem->update(m_delta_time);

					benchmark_subsystem->end_benchmark_category(subsystem->name(), "Engine Update");
				}
			}

			benchmark_subsystem->end_benchmark("Engine Update");
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
		if (m_play_state == PlayState::BeginPlay)
		{
			m_subsystem_manager->create_and_initialize_gameplay_subsystems();

			for (auto subsystem : m_subsystem_manager->get_subsystems())
			{
				subsystem->begin_play();
			}

			for (auto subsystem : m_subsystem_manager->get_gameplay_subsystems())
			{
				subsystem->begin_play();
			}

			m_accumulated_time = 0.0;
			m_play_state = PlayState::Playing;
		}

		if (m_play_state == PlayState::JustPaused)
		{
			//audioSubsystem->pauseAllSounds();

			m_play_state = PlayState::Paused;
		}

		if (m_play_state == PlayState::JustUnpaused)
		{
			//audioSubsystem->playAllSounds();

			m_play_state = PlayState::Playing;
		}

		if (m_play_state == PlayState::Playing)
		{
			// Fixed Update
			{
				auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmark_subsystem->start_benchmark("Fixed Update");

				// Add onto accumulated time
				m_accumulated_time += m_delta_time;

				while (m_accumulated_time >= m_time_step_fixed)
				{
					m_accumulated_time -= m_time_step_fixed;

					for (auto subsystem : m_subsystem_manager->get_gameplay_subsystems())
					{
						if (subsystem->should_fixed_update())
						{
							benchmark_subsystem->start_benchmark_category(subsystem->name(), "Fixed Update");

							subsystem->fixed_update(m_time_step_fixed);

							benchmark_subsystem->end_benchmark_category(subsystem->name(), "Fixed Update");
						}
					}
				}

				benchmark_subsystem->end_benchmark("Fixed Update");
			}

			// Update
			{
				auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmark_subsystem->start_benchmark("Update");

				for (auto subsystem : m_subsystem_manager->get_gameplay_subsystems())
				{
					if (subsystem->should_update())
					{
						benchmark_subsystem->start_benchmark_category(subsystem->name(), "Update");

						subsystem->update(m_delta_time);

						benchmark_subsystem->end_benchmark_category(subsystem->name(), "Update");
					}
				}

				benchmark_subsystem->end_benchmark("Update");
			}
		}

		// Render
		{
			auto benchmark_subsystem = get_subsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmark_subsystem->start_benchmark("Render");

			auto render_subsystem = m_subsystem_manager->get_render_subsystem();

			render_subsystem->render(m_delta_time);

			benchmark_subsystem->end_benchmark("Render");
		}

		if (m_play_state == PlayState::EndPlay)
		{
			// End play and cleanup gameplay subsystems
			for (auto subsystem : m_subsystem_manager->get_gameplay_subsystems())
			{
				subsystem->end_play();
			}

			for (auto subsystem : m_subsystem_manager->get_subsystems())
			{
				subsystem->end_play();
			}

			m_subsystem_manager->destroy_gameplay_subsystems();

			//audioSubsystem->stopAllSounds();

			m_accumulated_time = 0.0;
			m_play_state = PlayState::Stopped;
		}

		if (const auto window_subsystem = get_subsystem<window::WindowSubsystem>(); window_subsystem->should_primary_window_close())
		{
			m_running = false;
		}

		return m_running;
	}

	void Engine::deinitialize()
	{
		// Cleanup any running gameplay subsystems
		if (m_play_state == PlayState::Playing)
		{
			for (auto subsystem : m_subsystem_manager->get_gameplay_subsystems())
			{
				subsystem->end_play();
			}

			for (auto subsystem : m_subsystem_manager->get_subsystems())
			{
				subsystem->end_play();
			}

			m_subsystem_manager->destroy_gameplay_subsystems();
		}

		// Cleanup all engine subsystems
		m_subsystem_manager->destroy_engine_subsystems();

		// Clear Asset Registry
		assets::AssetRegistry::clear();

		m_subsystem_manager = nullptr;
	}

	void Engine::register_required_subsystems() const
	{
		register_subsystem<window::WindowSubsystem>();
		register_subsystem<core::SignalSubsystem>();
		register_subsystem<input::InputSubsystem>();
		register_subsystem<utility::PerformanceBenchmarkSubsystem>();
		register_subsystem<SettingsManager>();
		register_subsystem<EnkiTSSubsystem>();
		register_subsystem<audio::AudioSubsystem>();
		register_subsystem<ecs::EnTTSubsystem>();
		register_subsystem<scene::SceneGraphSubsystem>();
		register_subsystem<io::SceneSubsystem>();
		register_subsystem<rendering::CameraSubystem>();

		register_subsystem<ui::EditorUISubsystem>();

		register_subsystem<scene::SceneGraphGameplaySubsystem>();
	}

	void Engine::add_default_assets()
	{
		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		PuffinID meshId1 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath1)->id();
		//PuffinID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		PuffinID meshId3 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath3)->id();
		PuffinID meshId4 = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = fs::path() / "textures" / "cube.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath3 = fs::path() / "textures" / "space_engineer.ptexture";
		const fs::path& texturePath4 = fs::path() / "textures" / "statue.ptexture";
		const fs::path& texturePath5 = fs::path() / "textures" / "xsprite.ptexture";

		PuffinID textureId1 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath1)->id();
		PuffinID textureId2 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath2)->id();
		PuffinID textureId3 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath3)->id();
		PuffinID textureId4 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath4)->id();
		PuffinID textureId5 = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(texturePath5)->id();

		const fs::path& soundPath1 = fs::path() / "sounds" / "Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->add_asset<assets::SoundAsset>(soundPath1)->id();

		const fs::path shaderPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_vert.pshader";
		const fs::path shaderPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_frag.pshader";

		const auto shaderAsset1 = assets::AssetRegistry::get()->add_asset<assets::ShaderAsset>(shaderPath1);
		const auto shaderAsset2 = assets::AssetRegistry::get()->add_asset<assets::ShaderAsset>(shaderPath2);

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

		const auto materialInstAsset1 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(materialInstPath1);
		const auto materialInstAsset2 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(materialInstPath2);

		materialInstAsset1->getTexIDs()[0] = textureId1;

		materialInstAsset1->save();

		materialInstAsset2->getTexIDs()[0] = textureId2;

		materialInstAsset2->save();
	}

	void Engine::reimport_default_assets()
	{
		io::load_and_import_model(assets::AssetRegistry::get()->project_root() / "model_backups/cube.obj", "meshes");
		io::load_and_import_model(assets::AssetRegistry::get()->project_root() / "model_backups/space_engineer.obj", "meshes");
		//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
		io::load_and_import_model(assets::AssetRegistry::get()->project_root() / "model_backups/chalet.obj", "meshes");

		io::load_and_import_texture(assets::AssetRegistry::get()->project_root() / "texture_backups/chalet.jpg", "textures");
		io::load_and_import_texture(assets::AssetRegistry::get()->project_root() / "texture_backups/cube.png", "textures");
		io::load_and_import_texture(assets::AssetRegistry::get()->project_root() / "texture_backups/space_engineer.jpg", "textures");
		io::load_and_import_texture(assets::AssetRegistry::get()->project_root() / "texture_backups/statue.jpg", "textures");
		io::load_and_import_texture(assets::AssetRegistry::get()->project_root() / "texture_backups/xsprite.png", "textures");
	}

	void Engine::load_and_resave_assets()
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

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

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
				asset->load();
				asset->save();
				asset->unload();
			}
		}
	}

	void Engine::default_scene()
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::get()->content_root();

		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath1)->id();
		//const PuffinID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		const PuffinID meshId3 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath3)->id();
		const PuffinID meshId4 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "cube.ptexture";

		const PuffinID textureId1 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath1)->id();
		const PuffinID textureId2 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath2)->id();

		//const fs::path& soundPath1 = "sounds/Select 1.wav";

		//PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->get_asset<assets::MaterialInstanceAsset>(materialInstPath1)->id();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->get_asset<assets::MaterialInstanceAsset>(materialInstPath2)->id();

		auto registry = get_subsystem<ecs::EnTTSubsystem>()->registry();
		const auto scene_graph = get_subsystem<scene::SceneGraphSubsystem>();

		auto house_node = scene_graph->add_node<rendering::MeshNode>();
		house_node->set_name("House");
		house_node->set_position({ 2.0f, 0.0f, 0.0f });
		house_node->set_mesh_asset_id(meshId1);
		house_node->set_mat_asset_id(materialInstId1);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.set_name("Sphere");
		sphere.set_position({ -1.0f, -0.0f, 0.0f });
		sphere.set_mesh_asset_id(meshId2);
		sphere.set_mat_asset_id(materialInstId1);*/

		auto cube_1 = scene_graph->add_node<rendering::MeshNode>();
		cube_1->name() = "Cube_1";
		cube_1->set_position({ 0.0f });
		cube_1->set_mesh_asset_id(meshId3);
		cube_1->set_mat_asset_id(materialInstId1);

		auto cube_2 = scene_graph->add_node<rendering::MeshNode>();
		cube_2->set_name("Cube_2");
		cube_2->set_position({ -1.75f, -5.0f, 0.0f });
		cube_2->set_mesh_asset_id(meshId3);
		cube_2->set_mat_asset_id(materialInstId1);

		auto plane = scene_graph->add_node<rendering::MeshNode>();
		plane->set_name("Plane");
		plane->set_position({ 0.0f, -10.0f, 0.0f });
		plane->set_scale({ 50.0f, 1.0f, 50.0f });
		plane->set_mesh_asset_id(meshId3);
		plane->set_mat_asset_id(materialInstId1);

		auto dir_light = scene_graph->add_node<rendering::LightNode3D>();
		dir_light->set_name("Directional Light");
		dir_light->set_position({ 0.0f, 10.0f, 0.0f });
		dir_light->set_color({ .05f });
		dir_light->set_ambient_intensity(.0f);
		dir_light->set_light_type(rendering::LightType::Directional);
		dir_light->set_ambient_intensity(0.f);
		dir_light->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(dir_light->entity(), [&](auto& shadow) 
			{ shadow.width = 8192; shadow.height = 8192; shadow.bias_min = 0.3f; shadow.bias_max = 0.5f; });

		update_transform_orientation(*dir_light->transform_3d(), { 0.0f, -90.0f, 0.0f });

		auto dir_light_mesh = scene_graph->add_child_node<rendering::MeshNode>(dir_light->id());
		dir_light_mesh->set_scale({ 0.25f });
		dir_light_mesh->set_mesh_asset_id(meshId3);
		dir_light_mesh->set_mat_asset_id(materialInstId1);

		auto spot_light = scene_graph->add_node<rendering::LightNode3D>();
		spot_light->set_name("Spot Light");
		spot_light->set_position({ -10.0f, 5.0f, 0.0f });
		spot_light->set_light_type(rendering::LightType::Spot);
		spot_light->set_color({ 0.5f, 0.5f, 1.0f });
		spot_light->set_ambient_intensity(0.f);
		spot_light->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(spot_light->entity(), [&](auto& shadow) { shadow.width = 8192; shadow.height = 8192; });

		auto spot_light_mesh = scene_graph->add_child_node<rendering::MeshNode>(spot_light->id());
		spot_light_mesh->set_scale({ 0.25f });
		spot_light_mesh->set_mesh_asset_id(meshId3);
		spot_light_mesh->set_mat_asset_id(materialInstId1);

		auto spot_light_2 = scene_graph->add_node<rendering::LightNode3D>();
		spot_light_2->set_name("Spot Light 2");
		spot_light_2->set_position({ 10.0f, 5.0f, 0.0f });
		spot_light_2->set_light_type(rendering::LightType::Spot);
		spot_light_2->set_color({ 1.0f, 0.5f, 0.5f });
		spot_light_2->set_ambient_intensity(0.f);
		spot_light_2->add_component<rendering::ShadowCasterComponent>();
		registry->patch<rendering::ShadowCasterComponent>(spot_light_2->entity(), [&](auto& shadow) { shadow.width = 8192; shadow.height = 8192; });

		update_transform_orientation(*spot_light_2->transform_3d(), { 0.0f, 180.0f, 0.0f });

		auto spot_light_mesh_2 = scene_graph->add_child_node<rendering::MeshNode>(spot_light_2->id());
		spot_light_mesh_2->set_scale({ 0.25f });
		spot_light_mesh_2->set_mesh_asset_id(meshId3);
		spot_light_mesh_2->set_mat_asset_id(materialInstId1);

		//auto& script = registry->emplace<scripting::AngelScriptComponent>(entities[0]);
		//script.name = "ExampleScript";
		//script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::physics_scene_3d()
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::get()->content_root();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const PuffinID meshId1 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath1)->id();
		const PuffinID meshId2 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath2)->id();
		const PuffinID meshId3 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath3)->id();
		const PuffinID meshId4 = assets::AssetRegistry::get()->get_asset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		const PuffinID textureId1 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath1)->id();
		const PuffinID textureId2 = assets::AssetRegistry::get()->get_asset<assets::TextureAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		PuffinID soundId1 = assets::AssetRegistry::get()->get_asset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_chalet.pmaterialinst";

		PuffinID materialInstId1 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(materialInstPath1)->id();
		PuffinID materialInstId2 = assets::AssetRegistry::get()->add_asset<assets::MaterialInstanceAsset>(materialInstPath2)->id();

		const auto scene_graph = get_subsystem<scene::SceneGraphSubsystem>();

		// Light node

		auto light = scene_graph->add_node<rendering::LightNode3D>();
		light->position().y = 50.0f;
		light->set_light_type(rendering::LightType::Directional);
		light->set_ambient_intensity(0.01f);

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

		auto floor_body = scene_graph->add_node<physics::RigidbodyNode3D>();

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

	void Engine::procedural_scene()
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
        if (m_play_state == PlayState::Stopped)
        {
            m_play_state = PlayState::BeginPlay;
        }
		else if (m_play_state == PlayState::Playing)
        {
	        m_play_state = PlayState::JustPaused;
        }
        else if (m_play_state == PlayState::Paused)
        {
	        m_play_state = PlayState::JustUnpaused;
        }
	}

	void Engine::restart()
	{
		if (m_play_state == PlayState::Playing || m_play_state == PlayState::Paused || m_play_state == PlayState::Stopped)
		{
			m_play_state = PlayState::EndPlay;
		}
	}

	void Engine::exit()
	{
		m_running = false;
	}

	void Engine::update_delta_time(double sampled_time)
	{
		m_last_time = m_current_time;
		m_current_time = sampled_time;
		m_delta_time = m_current_time - m_last_time;

		if (m_frame_rate_max > 0)
		{
			const double deltaTimeMax = 1.0 / m_frame_rate_max;
			double idleStartTime = 0.0;

			while (m_delta_time < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				m_current_time = glfwGetTime();
				m_delta_time = m_current_time - m_last_time;
			}
		}
	}
}
