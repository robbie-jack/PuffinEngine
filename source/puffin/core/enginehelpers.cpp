#include "puffin/core/enginehelpers.h"

#include "puffin/assets/assetimporters.h"
#include "puffin/assets/materialasset.h"
#include "puffin/assets/shaderasset.h"
#include "puffin/assets/soundasset.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/assets/textureasset.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/components/scripting/angelscriptcomponent.h"
#include "puffin/core/engine.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/editor/ui/editoruisubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/nodes/physics/rigidbodynode3d.h"
#include "puffin/nodes/rendering/lightnode3d.h"
#include "puffin/nodes/rendering/meshnode.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/scene/scenesubsystem.h"
#include "puffin/utility/performancebenchmarksubsystem.h"
#include "puffin/window/windowsubsystem.h"

namespace puffin::core
{
	void RegisterAssetTypes(const io::ProjectFile& projectFile, const std::filesystem::path& projectPath)
	{
		assets::AssetRegistry::Get()->Initialize(projectFile, projectPath);
		assets::AssetRegistry::Get()->RegisterAssetType<assets::StaticMeshAsset>();
		assets::AssetRegistry::Get()->RegisterAssetType<assets::TextureAsset>();
		assets::AssetRegistry::Get()->RegisterAssetType<assets::SoundAsset>();
		assets::AssetRegistry::Get()->RegisterAssetType<assets::ShaderAsset>();
		assets::AssetRegistry::Get()->RegisterAssetType<assets::MaterialAsset>();
		assets::AssetRegistry::Get()->RegisterAssetType<assets::MaterialInstanceAsset>();
	}

	void RegisterComponents(const std::shared_ptr<Engine>& engine)
	{
		auto sceneSubsystem = engine->GetSubsystem<io::SceneSubsystem>();

		sceneSubsystem->register_component<TransformComponent2D>();
		sceneSubsystem->register_component<TransformComponent3D>();
		sceneSubsystem->register_component<rendering::StaticMeshComponent3D>();
		sceneSubsystem->register_component<rendering::PointLightComponent3D>();
		sceneSubsystem->register_component<rendering::SpotLightComponent3D>();
		sceneSubsystem->register_component<rendering::DirectionalLightComponent3D>();
		sceneSubsystem->register_component<rendering::ShadowCasterComponent3D>();
		sceneSubsystem->register_component<rendering::CameraComponent3D>();
		sceneSubsystem->register_component<scripting::AngelScriptComponent>();
		sceneSubsystem->register_component<physics::RigidbodyComponent2D>();
		sceneSubsystem->register_component<physics::BoxComponent2D>();
		sceneSubsystem->register_component<physics::CircleComponent2D>();
		sceneSubsystem->register_component<physics::RigidbodyComponent3D>();
		sceneSubsystem->register_component<physics::BoxComponent3D>();
		sceneSubsystem->register_component<physics::SphereComponent3D>();
	}

	void RegisterRequiredSubsystems(const std::shared_ptr<Engine>& engine)
	{
		// Engine Subsystems
		engine->RegisterSubsystem<window::WindowSubsystem>();
		engine->RegisterSubsystem<core::SignalSubsystem>();
		engine->RegisterSubsystem<input::InputSubsystem>();
		engine->RegisterSubsystem<utility::PerformanceBenchmarkSubsystem>();
		engine->RegisterSubsystem<SettingsManager>();
		engine->RegisterSubsystem<EnkiTSSubsystem>();
		engine->RegisterSubsystem<audio::AudioSubsystem>();
		engine->RegisterSubsystem<ecs::EnTTSubsystem>();
		engine->RegisterSubsystem<scene::SceneGraphSubsystem>();
		engine->RegisterSubsystem<io::SceneSubsystem>();
		engine->RegisterSubsystem<rendering::CameraSubystem>();

		// Editor Subsystems
		engine->RegisterSubsystem<ui::EditorUISubsystem>();

		// Gameplay Subsystems
		engine->RegisterSubsystem<scene::SceneGraphGameplaySubsystem>();
	}

	void AddDefaultAssets()
	{
		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		UUID meshId1 = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(meshPath1)->GetID();
		//PuffinID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		UUID meshId3 = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(meshPath3)->GetID();
		UUID meshId4 = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = fs::path() / "textures" / "cube.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath3 = fs::path() / "textures" / "space_engineer.ptexture";
		const fs::path& texturePath4 = fs::path() / "textures" / "statue.ptexture";
		const fs::path& texturePath5 = fs::path() / "textures" / "xsprite.ptexture";

		UUID textureId1 = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(texturePath1)->GetID();
		UUID textureId2 = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(texturePath2)->GetID();
		UUID textureId3 = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(texturePath3)->GetID();
		UUID textureId4 = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(texturePath4)->GetID();
		UUID textureId5 = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(texturePath5)->GetID();

		const fs::path& soundPath1 = fs::path() / "sounds" / "Select 1.wav";

		UUID soundId1 = assets::AssetRegistry::Get()->AddAsset<assets::SoundAsset>(soundPath1)->GetID();

		const fs::path shaderPath1 = fs::path() / "materials" / "forward_shading" / "forward_shading_vert.pshader";
		const fs::path shaderPath2 = fs::path() / "materials" / "forward_shading" / "forward_shading_frag.pshader";

		const auto shaderAsset1 = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(shaderPath1);
		const auto shaderAsset2 = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(shaderPath2);

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

		const auto materialInstAsset1 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			materialInstPath1);
		const auto materialInstAsset2 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			materialInstPath2);

		materialInstAsset1->GetTexIDs()[0] = textureId1;

		materialInstAsset1->Save();

		materialInstAsset2->GetTexIDs()[0] = textureId2;

		materialInstAsset2->Save();
	}

	void ImportDefaultAssets()
	{
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups/cube.obj", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups/space_engineer.obj","meshes");
		//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups/chalet.obj", "meshes");

		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups/chalet.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups/cube.png", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups/space_engineer.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups/statue.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups/xsprite.png", "textures");
	}

	void LoadAndSaveAssets()
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
			if (const auto asset = assets::AssetRegistry::Get()->GetAsset(path); asset != nullptr)
			{
				asset->Load();
				asset->Save();
				asset->Unload();
			}
		}
	}

	void SetupDefaultScene(const std::shared_ptr<Engine>& engine)
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		const fs::path& meshPath1 = fs::path() / "meshes" / "chalet.pstaticmesh";
		//const fs::path& meshPath2 = fs::path() / "meshes" / "sphere.pstaticmesh";
		const fs::path& meshPath3 = fs::path() / "meshes" / "cube.pstaticmesh";
		const fs::path& meshPath4 = fs::path() / "meshes" / "space_engineer.pstaticmesh";

		const UUID meshId1 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath1)->GetID();
		//const PuffinID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		const UUID meshId3 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath3)->GetID();
		const UUID meshId4 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = fs::path() / "textures" / "chalet.ptexture";
		const fs::path& texturePath2 = fs::path() / "textures" / "cube.ptexture";

		const UUID textureId1 = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texturePath1)->GetID();
		const UUID textureId2 = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texturePath2)->GetID();

		//const fs::path& soundPath1 = "sounds/Select 1.wav";

		//PuffinID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		UUID materialInstId1 = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			materialInstPath1)->GetID();
		UUID materialInstId2 = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			materialInstPath2)->GetID();

		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->registry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		auto houseNode = sceneGraph->add_node<rendering::MeshNode>();
		houseNode->set_name("House");
		houseNode->set_position({ 2.0f, 0.0f, 0.0f });
		houseNode->set_mesh_asset_id(meshId1);
		houseNode->set_mat_asset_id(materialInstId1);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.set_name("Sphere");
		sphere.set_position({ -1.0f, -0.0f, 0.0f });
		sphere.set_mesh_asset_id(meshId2);
		sphere.set_mat_asset_id(materialInstId1);*/

		auto cube1 = sceneGraph->add_node<rendering::MeshNode>();
		cube1->name() = "Cube_1";
		cube1->set_position({ 0.0f });
		cube1->set_mesh_asset_id(meshId3);
		cube1->set_mat_asset_id(materialInstId1);

		auto cube2 = sceneGraph->add_node<rendering::MeshNode>();
		cube2->set_name("Cube_2");
		cube2->set_position({ -1.75f, -5.0f, 0.0f });
		cube2->set_mesh_asset_id(meshId3);
		cube2->set_mat_asset_id(materialInstId1);

		auto plane = sceneGraph->add_node<rendering::MeshNode>();
		plane->set_name("Plane");
		plane->set_position({ 0.0f, -10.0f, 0.0f });
		plane->set_scale({ 50.0f, 1.0f, 50.0f });
		plane->set_mesh_asset_id(meshId3);
		plane->set_mat_asset_id(materialInstId1);

		/*auto dirLight = sceneGraph->add_node<rendering::LightNode3D>();
		dirLight->set_name("Directional Light");
		dirLight->set_position({ 0.0f, 10.0f, 0.0f });
		dirLight->SetColor({ .05f });
		dirLight->SetAmbientIntensity(.0f);
		dirLight->set_light_type(rendering::LightType::Directional);
		dirLight->SetAmbientIntensity(0.f);
		dirLight->add_component<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(dirLight->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
			shadow.biasMin = 0.3f;
			shadow.biasMax = 0.5f;
		});*/

		/*UpdateTransformOrientation(*dirLight->transform_3d(), { 0.0f, -90.0f, 0.0f });

		auto dirLightMesh = sceneGraph->add_child_node<rendering::MeshNode>(dirLight->id());
		dirLightMesh->set_scale({ 0.25f });
		dirLightMesh->set_mesh_asset_id(meshId3);
		dirLightMesh->set_mat_asset_id(materialInstId1);

		auto spotLight = sceneGraph->add_node<rendering::LightNode3D>();
		spotLight->set_name("Spot Light");
		spotLight->set_position({ -10.0f, 5.0f, 0.0f });
		spotLight->set_light_type(rendering::LightType::Spot);
		spotLight->SetColor({ 0.5f, 0.5f, 1.0f });
		spotLight->SetAmbientIntensity(0.f);
		spotLight->add_component<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(spotLight->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		auto spotLightMesh = sceneGraph->add_child_node<rendering::MeshNode>(spotLight->id());
		spotLightMesh->set_scale({ 0.25f });
		spotLightMesh->set_mesh_asset_id(meshId3);
		spotLightMesh->set_mat_asset_id(materialInstId1);

		auto spotLight2 = sceneGraph->add_node<rendering::LightNode3D>();
		spotLight2->set_name("Spot Light 2");
		spotLight2->set_position({ 10.0f, 5.0f, 0.0f });
		spotLight2->set_light_type(rendering::LightType::Spot);
		spotLight2->SetColor({ 1.0f, 0.5f, 0.5f });
		spotLight2->SetAmbientIntensity(0.f);
		spotLight2->add_component<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(spotLight2->entity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		UpdateTransformOrientation(*spotLight2->transform_3d(), { 0.0f, 180.0f, 0.0f });

		auto spotLightMesh2 = sceneGraph->add_child_node<rendering::MeshNode>(spotLight2->id());
		spotLightMesh2->set_scale({ 0.25f });
		spotLightMesh2->set_mesh_asset_id(meshId3);
		spotLightMesh2->set_mat_asset_id(materialInstId1);*/

		//auto& script = registry->emplace<scripting::AngelScriptComponent>(entities[0]);
		//script.name = "ExampleScript";
		//script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void SetupDefaultPhysicsScene3D(const std::shared_ptr<Engine>& engine)
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		const UUID meshId1 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath1)->GetID();
		const UUID meshId2 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath2)->GetID();
		const UUID meshId3 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath3)->GetID();
		const UUID meshId4 = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshPath4)->GetID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		const UUID textureId1 = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texturePath1)->GetID();
		const UUID textureId2 = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(texturePath2)->GetID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = assets::AssetRegistry::Get()->GetAsset<assets::SoundAsset>(soundPath1)->GetID();

		const fs::path materialInstPath1 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
		const fs::path materialInstPath2 = fs::path() / "materials" / "forward_shading" /
			"forward_shading_chalet.pmaterialinst";

		UUID materialInstId1 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			materialInstPath1)->GetID();
		UUID materialInstId2 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			materialInstPath2)->GetID();

		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		// Light node

		/*auto light = sceneGraph->add_node<rendering::LightNode3D>();
		light->position().y = 50.0f;
		light->set_light_type(rendering::LightType::Directional);
		light->SetAmbientIntensity(0.01f);*/

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
}
