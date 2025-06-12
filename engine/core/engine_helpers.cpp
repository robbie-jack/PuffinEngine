#include "core/engine_helpers.h"

#include "audio/audio_subsystem.h"
#include "component/physics/2d/box_component_2d.h"
#include "component/physics/2d/circle_component_2d.h"
#include "component/physics/2d/rigidbody_component_2d.h"
#include "component/physics/2d/velocity_component_2d.h"
#include "component/physics/3d/box_component_3d.h"
#include "component/physics/3d/rigidbody_component_3d.h"
#include "component/physics/3d/sphere_component_3d.h"
#include "component/physics/3d/velocity_component_3d.h"
#include "component/procedural/3d/procedural_ico_sphere_component_3d.h"
#include "component/procedural/3d/procedural_plane_component_3d.h"
#include "component/procedural/3d/procedural_terrain_component_3d.h"
#include "component/procedural/3d/procedural_uv_sphere_component_3d.h"
#include "component/rendering/2d/camera_component_2d.h"
#include "component/rendering/2d/sprite_component_2d.h"
#include "component/rendering/3d/camera_component_3d.h"
#include "component/rendering/3d/directional_light_component_3d.h"
#include "component/rendering/3d/point_light_component_3d.h"
#include "component/rendering/3d/procedural_mesh_component_3d.h"
#include "component/rendering/3d/shadow_caster_component_3d.h"
#include "component/rendering/3d/spot_light_component_3d.h"
#include "component/rendering/3d/static_mesh_component_3d.h"
#include "component/transform_component_2d.h"
#include "component/transform_component_3d.h"
#include "component/procedural/3d/procedural_cube_component_3d.h"
#include "core/engine.h"
#include "core/enkits_subsystem.h"
#include "core/settings_manager.h"
#include "core/signal_subsystem.h"
#include "ecs/entt_subsystem.h"
#include "node/node.h"
#include "node/physics/2d/box_2d_node.h"
#include "node/physics/2d/rigidbody_2d_node.h"
#include "node/physics/3d/rigidbody_3d_node.h"
#include "node/rendering/2d/sprite_2d_node.h"
#include "node/rendering/3d/camera_3d_node.h"
#include "node/rendering/3d/directional_light_3d_node.h"
#include "node/rendering/3d/point_light_3d_node.h"
#include "node/rendering/3d/spot_light_3d_node.h"
#include "node/rendering/3d/static_mesh_3d_node.h"
#include "node/transform_2d_node.h"
#include "node/transform_3d_node.h"
#include "node/rendering/3d/light_3d_node.h"
#include "rendering/camera_subsystem.h"
#include "scene/scene_graph_gameplay_subsystem.h"
#include "scene/scene_graph_subsystem.h"
#include "scene/scene_serialization_subsystem.h"

namespace puffin::core
{
	const fs::path& gChaletMeshPath = fs::path() / "meshes" / "chalet.pstaticmesh";
	const fs::path& gCubeMeshPath = fs::path() / "meshes" / "cube.pstaticmesh";
	const fs::path& gSpaceEngineerMeshPath = fs::path() / "meshes" / "space_engineer.pstaticmesh";
	const fs::path& gSphereMeshPath = fs::path() / "meshes" / "sphere.pstaticmesh";

	const fs::path& gCubeTexturePath = fs::path() / "textures" / "cube.ptexture";
	const fs::path& gChaletTexturePath = fs::path() / "textures" / "chalet.ptexture";
	const fs::path& gSpaceEngineerTexturePath = fs::path() / "textures" / "space_engineer.ptexture";
	const fs::path& gStatueTexturePath = fs::path() / "textures" / "statue.ptexture";
	const fs::path& gXSpriteTexturePath = fs::path() / "textures" / "xsprite.ptexture";

	const fs::path& gSelectSoundPath = fs::path() / "sounds" / "Select 1.wav";

	const fs::path gForwardVertShaderPath = fs::path() / "materials" / "forward_shading" / "forward_shading_vert.pshader";
	const fs::path gForwardFragShaderPath = fs::path() / "materials" / "forward_shading" / "forward_shading_frag.pshader";

	const fs::path gForwardShadingDefaultMaterialInstPath = fs::path() / "materials" / "forward_shading" /
			"forward_shading_default.pmaterialinst";
	const fs::path gForwardShadinChaletMaterialInstPath = fs::path() / "materials" / "forward_shading" /
		"forward_shading_chalet.pmaterialinst";
	
	void RegisterRequiredSubsystems(const std::shared_ptr<Engine>& engine)
	{
		// Engine Subsystems
		engine->RegisterSubsystem<SignalSubsystem>();
		engine->RegisterSubsystem<SettingsManager>();
		engine->RegisterSubsystem<EnkiTSSubsystem>();
		engine->RegisterSubsystem<audio::AudioSubsystem>();
		engine->RegisterSubsystem<ecs::EnTTSubsystem>();
		engine->RegisterSubsystem<scene::SceneGraphSubsystem>();
		engine->RegisterSubsystem<scene::SceneSerializationSubsystem>();
		engine->RegisterSubsystem<rendering::CameraSubsystem>();

		// Gameplay Subsystems
		engine->RegisterSubsystem<scene::SceneGraphGameplaySubsystem>();
	}

	void RegisterComponentTypes2D()
	{
		reflection::RegisterType<TransformComponent2D>();

		reflection::RegisterType<rendering::CameraComponent2D>();
		reflection::RegisterType<rendering::SpriteComponent2D>();

		reflection::RegisterType<physics::BoxComponent2D>();
		reflection::RegisterType<physics::CircleComponent2D>();
		reflection::RegisterType<physics::RigidbodyComponent2D>();
		reflection::RegisterType<physics::VelocityComponent2D>();
	}

	void RegisterComponentTypes3D()
	{
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

	void RegisterNodeTypes2D()
	{
		reflection::RegisterType<Node>();

		reflection::RegisterType<Transform2DNode>();

		reflection::RegisterType<rendering::Sprite2DNode>();

		reflection::RegisterType<physics::Rigidbody2DNode>();
		reflection::RegisterType<physics::Shape2DNode>();
		reflection::RegisterType<physics::Box2DNode>();
	}

	void RegisterNodeTypes3D()
	{
		reflection::RegisterType<Node>();

		reflection::RegisterType<TransformNode3D>();

		reflection::RegisterType<rendering::LightNode3D>();
		reflection::RegisterType<rendering::PointLightNode3D>();
		reflection::RegisterType<rendering::SpotLightNode3D>();
		reflection::RegisterType<rendering::DirectionalLightNode3D>();

		reflection::RegisterType<rendering::CameraNode3D>();
		reflection::RegisterType<rendering::StaticMeshNode3D>();

		reflection::RegisterType<physics::RigidbodyNode3D>();
	}

	//void RegisterAssetTypes(const io::ProjectFile& projectFile, const std::filesystem::path& projectPath)
	//{
	//	assets::AssetRegistry::Get()->Initialize(projectFile, projectPath);
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::StaticMeshAsset>();
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::TextureAsset>();
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::SoundAsset>();
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::ShaderAsset>();
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::MaterialAsset>();
	//	assets::AssetRegistry::Get()->RegisterAssetType<assets::MaterialInstanceAsset>();
	//}

	//void AddDefaultAssets()
	//{
	//	UUID chaletMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
	//	UUID cubeMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();
	//	//UUID spaceEngineerMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gSpaceEngineerMeshPath)->GetID();
	//	//UUID sphereMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gSphereMeshPath)->GetID();

	//	UUID cubeTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();
	//	UUID chaletTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
	//	//UUID spaceEngineerTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gSpaceEngineerTexturePath)->GetID();
	//	//UUID statueTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gStatueTexturePath)->GetID();
	//	//UUID xSpriteTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gXSpriteTexturePath)->GetID();

	//	UUID selectSoundID = assets::AssetRegistry::Get()->AddAsset<assets::SoundAsset>(gSelectSoundPath)->GetID();

	//	const auto forwardVertShaderAsset = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(gForwardVertShaderPath);
	//	const auto forwardFragShaderAsset = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(gForwardFragShaderPath);

	//	const auto materialInstAsset1 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
	//		gForwardShadingDefaultMaterialInstPath);
	//	const auto materialInstAsset2 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
	//		gForwardShadinChaletMaterialInstPath);

	//	materialInstAsset1->GetTexIDs()[0] = cubeTexID;

	//	materialInstAsset1->Save();

	//	materialInstAsset2->GetTexIDs()[0] = chaletTexID;

	//	materialInstAsset2->Save();
	//}

	//void ImportDefaultAssets()
	//{
	//	io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "cube.obj", "meshes");
	//	io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "space_engineer.obj","meshes");
	//	//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
	//	io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "chalet.obj", "meshes");

	//	io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "chalet.jpg", "textures");
	//	io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "cube.png", "textures");
	//	io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "space_engineer.jpg", "textures");
	//	io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "statue.jpg", "textures");
	//	io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "xsprite.png", "textures");
	//}

	/*void LoadAndSaveAssets()
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
			gChaletMeshPath, gSphereMeshPath, gCubeMeshPath, gSpaceEngineerMeshPath,
			gChaletTexturePath, gCubeTexturePath, gSpaceEngineerTexturePath, gStatueTexturePath, gXSpriteTexturePath,
			gForwardFragShaderPath, gForwardVertShaderPath,
			gForwardShadingDefaultMaterialInstPath, gForwardShadinChaletMaterialInstPath,
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
	}*/

	void SetupDefaultScene2D(const std::shared_ptr<Engine>& engine)
	{
		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		// Box
		{
			auto* box = sceneGraph->AddNode<Transform2DNode>("Box");

			auto sprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", box->GetID());
			sprite->SetColour({ 1.f, 0.f, 0.f });
		}
	}

	void SetupDefaultPhysicsScene2D(const std::shared_ptr<Engine>& engine)
	{
		/*const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadingDefaultMaterialInstPath)->GetID();
		UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadinChaletMaterialInstPath)->GetID();*/

		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		const Vector2f wallHalfExtent = { 100.0f, 100.f };
		float wallHalfWidth = 0.5f;

		// Floor Node
		{
			auto* floor = sceneGraph->AddNode<physics::Rigidbody2DNode>("Floor");
			floor->SetGlobalPosition({ 0.0f, -wallHalfExtent.y });

			auto* floorBox = sceneGraph->AddChildNode<physics::Box2DNode>("Box", floor->GetID());
			floorBox->SetHalfExtent({ wallHalfExtent.x, wallHalfWidth });

			auto* floorSprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", floor->GetID());
			floorSprite->SetScale({ wallHalfExtent.x * 2.f, 1.0f });
			floorSprite->SetOffset({ -wallHalfExtent.x, 0.0f });
		}

		// Left Wall Node
		{
			auto* leftWall = sceneGraph->AddNode<physics::Rigidbody2DNode>("Left Wall");
			leftWall->SetGlobalPosition({ -wallHalfExtent.x, 0.0f });

			auto* leftWallBox = sceneGraph->AddChildNode<physics::Box2DNode>("Box", leftWall->GetID());
			leftWallBox->SetHalfExtent({ wallHalfWidth, wallHalfExtent.y });

			auto* leftWallSprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", leftWall->GetID());
			leftWallSprite->SetScale({ 1.0f, wallHalfExtent.y * 2.f });
			leftWallSprite->SetOffset({ 0.0f, -wallHalfExtent.y });
		}

		// Right Wall Node
		{
			auto* rightWall = sceneGraph->AddNode<physics::Rigidbody2DNode>("Right Wall");
			rightWall->SetGlobalPosition({ wallHalfExtent.x, 0.0f });

			auto* rightWallBox = sceneGraph->AddChildNode<physics::Box2DNode>("Box", rightWall->GetID());
			rightWallBox->SetHalfExtent({ wallHalfWidth, wallHalfExtent.y });

			auto* rightWallSprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", rightWall->GetID());
			rightWallSprite->SetScale({ 1.0f, wallHalfExtent.y * 2.f });
			rightWallSprite->SetOffset({ 0.0f, -wallHalfExtent.y });
		}

		// Ceiling Node
		{
			auto* ceiling = sceneGraph->AddNode<physics::Rigidbody2DNode>("Ceiling");
			ceiling->SetGlobalPosition({ 0.0f, wallHalfExtent.y});

			auto* ceilingBox = sceneGraph->AddChildNode<physics::Box2DNode>("Box", ceiling->GetID());
			ceilingBox->SetHalfExtent({ wallHalfExtent.x, wallHalfWidth });

			auto* ceilingSprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", ceiling->GetID());
			ceilingSprite->SetScale({ wallHalfExtent.x * 2.f, 1.0f });
			ceilingSprite->SetOffset({ -wallHalfExtent.x, 0.0f });
		}

		std::vector<Vector3f> colours =
		{
			{ 1.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f},
			{ 0.0f, 0.0f, 1.0f},
			{ 1.0f, 1.0f, 0.0f },
			{ 1.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 1.0f},
		};

		// Box Nodes
		{
			constexpr int numBodies = 500;

			const Vector2f bodyHalfExtent = { 0.5f, 0.5f };
			const Vector2f bodyPositionHalfRange = { wallHalfExtent.x * 0.9f, wallHalfExtent.y * 0.9f };
			double velocityMax = 50.0;

			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution posXDist(-bodyPositionHalfRange.x, bodyPositionHalfRange.x);
			std::uniform_real_distribution posYDist(-bodyPositionHalfRange.y, bodyPositionHalfRange.y);
			std::uniform_real_distribution velDist(-velocityMax, velocityMax);

			for (int i = 0; i < numBodies; ++i)
			{
				auto* body = sceneGraph->AddNode<physics::Rigidbody2DNode>("Body #" + std::to_string(i));
				body->SetGlobalPosition({ posXDist(mt), posYDist(mt) });
				body->SetBodyType(physics::BodyType::Dynamic);
				body->SetFriction(0.0f);
				body->SetLinearVelocity({ static_cast<float>(velDist(mt)), static_cast<float>(velDist(mt)) });

				auto* bodyBox = sceneGraph->AddChildNode<physics::Box2DNode>("Box", body->GetID());
				bodyBox->SetHalfExtent(bodyHalfExtent);

				auto* bodySprite = sceneGraph->AddChildNode<rendering::Sprite2DNode>("Sprite", body->GetID());
				bodySprite->SetColour(colours[i % colours.size()]);
			}
		}
	}

	void SetupDefaultScene3D(const std::shared_ptr<Engine>& engine)
	{
		//// Get assets
		//fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		//const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		//const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		//const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		//const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		//UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
		//	gForwardShadingDefaultMaterialInstPath)->GetID();
		//UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
		//	gForwardShadinChaletMaterialInstPath)->GetID();

		//auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		//const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		//auto* houseNode = sceneGraph->AddNode<rendering::StaticMeshNode3D>("House");
		//houseNode->SetPosition({ 2.0f, 0.0f, 0.0f });
		//houseNode->SetMeshID(chaletMeshID);
		//houseNode->SetMaterialID(defaultMaterialInstID);

		///*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		//sphere.SetName("Sphere");
		//sphere.SetPosition({ -1.0f, -0.0f, 0.0f });
		//sphere.SetMeshID(meshId2);
		//sphere.SetMaterialID(materialInstId1);*/

		//auto* cube1 = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Cube 1");
		//cube1->SetPosition({ 0.0f });
		//cube1->SetMeshID(cubeMeshID);
		//cube1->SetMaterialID(defaultMaterialInstID);

		//auto* cube2 = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Cube 2");
		//cube2->SetPosition({ -1.75f, -5.0f, 0.0f });
		//cube2->SetMeshID(cubeMeshID);
		//cube2->SetMaterialID(defaultMaterialInstID);

		//auto* plane = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Plane");
		//plane->SetPosition({ 0.0f, -10.0f, 0.0f });
		//plane->SetScale({ 50.0f, 1.0f, 50.0f });
		//plane->SetMeshID(cubeMeshID);
		//plane->SetMaterialID(defaultMaterialInstID);

		//auto* dirLight = sceneGraph->AddNode<rendering::DirectionalLightNode3D>("Directional Light");
		//dirLight->SetPosition({ 0.0f, 10.0f, 0.0f });
		//dirLight->SetColor({ .05f });
		//dirLight->SetAmbientIntensity(.0f);
		//dirLight->AddComponent<rendering::ShadowCasterComponent3D>();
		//registry->patch<rendering::ShadowCasterComponent3D>(dirLight->GetEntity(), [&](auto& shadow)
		//{
		//	shadow.width = 8192;
		//	shadow.height = 8192;
		//	shadow.biasMin = 0.3f;
		//	shadow.biasMax = 0.5f;
		//});

		//UpdateTransformOrientation(dirLight->Transform(), { 0.0f, -90.0f, 0.0f });

		//auto* dirLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Directional Light Mesh", dirLight->GetID());
		//dirLightMesh->SetScale({ 0.25f });
		//dirLightMesh->SetMeshID(cubeMeshID);
		//dirLightMesh->SetMaterialID(defaultMaterialInstID);

		//auto* spotLight = sceneGraph->AddNode<rendering::SpotLightNode3D>("Spot Light");
		//spotLight->SetPosition({ -10.0f, 5.0f, 0.0f });
		//spotLight->SetColor({ 0.5f, 0.5f, 1.0f });
		//spotLight->SetAmbientIntensity(0.f);
		//spotLight->AddComponent<rendering::ShadowCasterComponent3D>();
		//registry->patch<rendering::ShadowCasterComponent3D>(spotLight->GetEntity(), [&](auto& shadow)
		//{
		//	shadow.width = 8192;
		//	shadow.height = 8192;
		//});

		//auto* spotLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Spot Light Mesh", spotLight->GetID());
		//spotLightMesh->SetScale({ 0.25f });
		//spotLightMesh->SetMeshID(cubeMeshID);
		//spotLightMesh->SetMaterialID(defaultMaterialInstID);

		//auto* spotLight2 = sceneGraph->AddNode<rendering::SpotLightNode3D>("Spot Light 2");
		//spotLight2->SetPosition({ 10.0f, 5.0f, 0.0f });
		//spotLight2->SetColor({ 1.0f, 0.5f, 0.5f });
		//spotLight2->SetAmbientIntensity(0.f);
		//spotLight2->AddComponent<rendering::ShadowCasterComponent3D>();
		//registry->patch<rendering::ShadowCasterComponent3D>(spotLight2->GetEntity(), [&](auto& shadow)
		//{
		//	shadow.width = 8192;
		//	shadow.height = 8192;
		//});

		//UpdateTransformOrientation(spotLight2->Transform(), { 0.0f, 180.0f, 0.0f });

		//auto* spotLightMesh2 = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Spot Light 2 Mesh", spotLight2->GetID());
		//spotLightMesh2->SetScale({ 0.25f });
		//spotLightMesh2->SetMeshID(cubeMeshID);
		//spotLightMesh2->SetMaterialID(defaultMaterialInstID);
	}

	void SetupDefaultPhysicsScene3D(const std::shared_ptr<Engine>& engine)
	{
		//// Get assets
		//fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		//const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		//const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		//const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		//const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		//UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
		//	gForwardShadingDefaultMaterialInstPath)->GetID();
		//UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
		//	gForwardShadinChaletMaterialInstPath)->GetID();

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

		auto floorBody = sceneGraph->AddNode<physics::RigidbodyNode3D>("Floor");

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
