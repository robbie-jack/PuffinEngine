#include "puffin/core/enginehelpers.h"

#include "puffin/assets/assetimporters.h"
#include "puffin/assets/materialasset.h"
#include "puffin/assets/shaderasset.h"
#include "puffin/assets/soundasset.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/assets/textureasset.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/procedural/3d/proceduralicospherecomponent3d.h"
#include "puffin/components/procedural/3d/proceduralplanecomponent3d.h"
#include "puffin/components/procedural/3d/proceduralterraincomponent3d.h"
#include "puffin/components/procedural/3d/proceduraluvspherecomponent3d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/proceduralmeshcomponent3d.h"
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
#include "puffin/nodes/rendering/3d/directionallightnode3d.h"
#include "puffin/nodes/rendering/3d/pointlightnode3d.h"
#include "puffin/nodes/rendering/3d/spotlightnode3d.h"
#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/scene/scenegraphgameplaysubsystem.h"
#include "puffin/scene/sceneserializationsubsystem.h"
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

	void RegisterComponentTypes()
	{
		// Register 2D Types
		reflection::RegisterType<TransformComponent2D>();

		reflection::RegisterType<physics::BoxComponent2D>();
		reflection::RegisterType<physics::CircleComponent2D>();
		reflection::RegisterType<physics::RigidbodyComponent2D>();
		reflection::RegisterType<physics::VelocityComponent2D>();

		// Register 3D Types
		reflection::RegisterType<TransformComponent3D>();

		reflection::RegisterType<physics::BoxComponent3D>();
		reflection::RegisterType<physics::SphereComponent3D>();
		reflection::RegisterType<physics::RigidbodyComponent3D>();
		reflection::RegisterType<physics::VelocityComponent3D>();

		reflection::RegisterType<procedural::ProceduralPlaneComponent3D>();
		reflection::RegisterType<procedural::ProceduralCubeComponent3D>();
		reflection::RegisterType<procedural::ProceduralTerrainComponent3D>();
		reflection::RegisterType<procedural::ProceduralIcoSphereComponent3D>();
		reflection::RegisterType<procedural::ProceduralUVSphereComponent3D>();

		reflection::RegisterType<rendering::StaticMeshComponent3D>();
		reflection::RegisterType<rendering::ProceduralMeshComponent3D>();
		reflection::RegisterType<rendering::PointLightComponent3D>();
		reflection::RegisterType<rendering::SpotLightComponent3D>();
		reflection::RegisterType<rendering::DirectionalLightComponent3D>();
		reflection::RegisterType<rendering::ShadowCasterComponent3D>();

		reflection::RegisterType<rendering::CameraComponent3D>();
	}

	void RegisterComponentsForSerialization(const std::shared_ptr<Engine>& engine)
	{
		auto sceneSubsystem = engine->GetSubsystem<io::SceneSerializationSubsystem>();

		// 2D
		sceneSubsystem->RegisterComponent<TransformComponent2D>();

		sceneSubsystem->RegisterComponent<physics::BoxComponent2D>();
		sceneSubsystem->RegisterComponent<physics::CircleComponent2D>();
		sceneSubsystem->RegisterComponent<physics::RigidbodyComponent2D>();
		sceneSubsystem->RegisterComponent<physics::VelocityComponent2D>();

		// 3D
		sceneSubsystem->RegisterComponent<TransformComponent3D>();

		sceneSubsystem->RegisterComponent<physics::BoxComponent3D>();
		sceneSubsystem->RegisterComponent<physics::SphereComponent3D>();
		sceneSubsystem->RegisterComponent<physics::RigidbodyComponent3D>();
		sceneSubsystem->RegisterComponent<physics::VelocityComponent3D>();

		sceneSubsystem->RegisterComponent<procedural::ProceduralPlaneComponent3D>();
		sceneSubsystem->RegisterComponent<procedural::ProceduralCubeComponent3D>();
		sceneSubsystem->RegisterComponent<procedural::ProceduralTerrainComponent3D>();
		sceneSubsystem->RegisterComponent<procedural::ProceduralIcoSphereComponent3D>();
		sceneSubsystem->RegisterComponent<procedural::ProceduralUVSphereComponent3D>();

		sceneSubsystem->RegisterComponent<rendering::StaticMeshComponent3D>();
		sceneSubsystem->RegisterComponent<rendering::ProceduralMeshComponent3D>();
		sceneSubsystem->RegisterComponent<rendering::PointLightComponent3D>();
		sceneSubsystem->RegisterComponent<rendering::SpotLightComponent3D>();
		sceneSubsystem->RegisterComponent<rendering::DirectionalLightComponent3D>();
		sceneSubsystem->RegisterComponent<rendering::ShadowCasterComponent3D>();

		sceneSubsystem->RegisterComponent<rendering::CameraComponent3D>();
		
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
		engine->RegisterSubsystem<io::SceneSerializationSubsystem>();
		engine->RegisterSubsystem<rendering::CameraSubsystem>();

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

		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		auto* houseNode = sceneGraph->AddNode<rendering::StaticMeshNode3D>();
		houseNode->SetName("House");
		houseNode->SetPosition({ 2.0f, 0.0f, 0.0f });
		houseNode->SetMeshID(meshId1);
		houseNode->SetMaterialID(materialInstId1);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.SetName("Sphere");
		sphere.SetPosition({ -1.0f, -0.0f, 0.0f });
		sphere.SetMeshID(meshId2);
		sphere.SetMaterialID(materialInstId1);*/

		auto* cube1 = sceneGraph->AddNode<rendering::StaticMeshNode3D>();
		cube1->SetName("Cube_1");
		cube1->SetPosition({ 0.0f });
		cube1->SetMeshID(meshId3);
		cube1->SetMaterialID(materialInstId1);

		auto* cube2 = sceneGraph->AddNode<rendering::StaticMeshNode3D>();
		cube2->SetName("Cube_2");
		cube2->SetPosition({ -1.75f, -5.0f, 0.0f });
		cube2->SetMeshID(meshId3);
		cube2->SetMaterialID(materialInstId1);

		auto* plane = sceneGraph->AddNode<rendering::StaticMeshNode3D>();
		plane->SetName("Plane");
		plane->SetPosition({ 0.0f, -10.0f, 0.0f });
		plane->SetScale({ 50.0f, 1.0f, 50.0f });
		plane->SetMeshID(meshId3);
		plane->SetMaterialID(materialInstId1);

		auto* dirLight = sceneGraph->AddNode<rendering::DirectionalLightNode3D>();
		dirLight->SetName("Directional Light");
		dirLight->SetPosition({ 0.0f, 10.0f, 0.0f });
		dirLight->SetColor({ .05f });
		dirLight->SetAmbientIntensity(.0f);
		dirLight->SetAmbientIntensity(0.f);
		dirLight->AddComponent<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(dirLight->GetEntity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
			shadow.biasMin = 0.3f;
			shadow.biasMax = 0.5f;
		});

		UpdateTransformOrientation(dirLight->Transform(), { 0.0f, -90.0f, 0.0f });

		auto* dirLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>(dirLight->GetID());
		dirLightMesh->SetScale({ 0.25f });
		dirLightMesh->SetMeshID(meshId3);
		dirLightMesh->SetMaterialID(materialInstId1);

		auto* spotLight = sceneGraph->AddNode<rendering::SpotLightNode3D>();
		spotLight->SetName("Spot Light");
		spotLight->SetPosition({ -10.0f, 5.0f, 0.0f });
		spotLight->SetColor({ 0.5f, 0.5f, 1.0f });
		spotLight->SetAmbientIntensity(0.f);
		spotLight->AddComponent<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(spotLight->GetEntity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		auto* spotLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>(spotLight->GetID());
		spotLightMesh->SetScale({ 0.25f });
		spotLightMesh->SetMeshID(meshId3);
		spotLightMesh->SetMaterialID(materialInstId1);

		auto* spotLight2 = sceneGraph->AddNode<rendering::SpotLightNode3D>();
		spotLight2->SetName("Spot Light 2");
		spotLight2->SetPosition({ 10.0f, 5.0f, 0.0f });
		spotLight2->SetColor({ 1.0f, 0.5f, 0.5f });
		spotLight2->SetAmbientIntensity(0.f);
		spotLight2->AddComponent<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(spotLight2->GetEntity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		UpdateTransformOrientation(spotLight2->Transform(), { 0.0f, 180.0f, 0.0f });

		auto* spotLightMesh2 = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>(spotLight2->GetID());
		spotLightMesh2->SetScale({ 0.25f });
		spotLightMesh2->SetMeshID(meshId3);
		spotLightMesh2->SetMaterialID(materialInstId1);

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

		auto floorBody = sceneGraph->AddNode<physics::RigidbodyNode3D>();

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
