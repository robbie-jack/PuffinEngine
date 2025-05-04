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
#include "puffin/components/rendering/2d/spritecomponent2d.h"
#include "puffin/components/rendering/2d/cameracomponent2d.h"
#include "puffin/components/rendering/3d/cameracomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/proceduralmeshcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/core/engine.h"
#include "puffin/core/enkitssubsystem.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/core/signalsubsystem.h"
#include "puffin/editor/ui/editoruisubsystem.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/physics/rigidbodynode3d.h"
#include "puffin/nodes/rendering/3d/cameranode3d.h"
#include "puffin/nodes/rendering/3d/directionallightnode3d.h"
#include "puffin/nodes/rendering/3d/pointlightnode3d.h"
#include "puffin/nodes/rendering/3d/spotlightnode3d.h"
#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/scene/scenegraphgameplaysubsystem.h"
#include "puffin/scene/sceneserializationsubsystem.h"
#include "puffin/window/windowsubsystem.h"

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
		engine->RegisterSubsystem<io::SceneSerializationSubsystem>();
		engine->RegisterSubsystem<rendering::CameraSubsystem>();

		// Editor Subsystems
		engine->RegisterSubsystem<ui::EditorUISubsystem>();

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

		reflection::RegisterType<TransformNode2D>();
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

	void AddDefaultAssets()
	{
		UUID chaletMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		UUID cubeMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();
		//UUID spaceEngineerMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gSpaceEngineerMeshPath)->GetID();
		//UUID sphereMeshID = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(gSphereMeshPath)->GetID();

		UUID cubeTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();
		UUID chaletTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		//UUID spaceEngineerTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gSpaceEngineerTexturePath)->GetID();
		//UUID statueTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gStatueTexturePath)->GetID();
		//UUID xSpriteTexID = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(gXSpriteTexturePath)->GetID();

		UUID selectSoundID = assets::AssetRegistry::Get()->AddAsset<assets::SoundAsset>(gSelectSoundPath)->GetID();

		const auto forwardVertShaderAsset = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(gForwardVertShaderPath);
		const auto forwardFragShaderAsset = assets::AssetRegistry::Get()->AddAsset<assets::ShaderAsset>(gForwardFragShaderPath);

		const auto materialInstAsset1 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			gForwardShadingDefaultMaterialInstPath);
		const auto materialInstAsset2 = assets::AssetRegistry::Get()->AddAsset<assets::MaterialInstanceAsset>(
			gForwardShadinChaletMaterialInstPath);

		materialInstAsset1->GetTexIDs()[0] = cubeTexID;

		materialInstAsset1->Save();

		materialInstAsset2->GetTexIDs()[0] = chaletTexID;

		materialInstAsset2->Save();
	}

	void ImportDefaultAssets()
	{
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "cube.obj", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "space_engineer.obj","meshes");
		//io::loadAndImportModel(R"(C:\Projects\PuffinProject\model_backups\Sphere.dae)", "meshes");
		io::LoadAndImportModel(assets::AssetRegistry::Get()->GetProjectRoot() / "model_backups" / "chalet.obj", "meshes");

		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "chalet.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "cube.png", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "space_engineer.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "statue.jpg", "textures");
		io::LoadAndImportTexture(assets::AssetRegistry::Get()->GetProjectRoot() / "texture_backups" / "xsprite.png", "textures");
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
	}

	void SetupDefaultScene2D(const std::shared_ptr<Engine>& engine)
	{
		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		// Box
		{
			auto* box = sceneGraph->AddNode<TransformNode2D>("Box");
		}
	}

	void SetupDefaultPhysicsScene2D(const std::shared_ptr<Engine>& engine)
	{
		const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadingDefaultMaterialInstPath)->GetID();
		UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadinChaletMaterialInstPath)->GetID();

		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		// Light
		{
			auto* light = sceneGraph->AddNode<rendering::DirectionalLightNode3D>("Light");
			light->SetPosition({ 0.0f, 100.0f, 0.0f });
			light->SetColor({ 1.f });
			light->SetAmbientIntensity(.1f);

			//UpdateTransformOrientation(light->Transform(), { 0.0f, -90.0f, 0.0f });
		}

		const Vector2f wallHalfExtent = { 200.0f, 200.f };

		// Floor Node
		{
			auto* floor = sceneGraph->AddNode<TransformNode3D>("Floor");
			floor->SetPosition({ 0.0f, -wallHalfExtent.y, 0.0f });

			auto& rb = floor->AddComponent<physics::RigidbodyComponent2D>();
			auto& box = floor->AddComponent<physics::BoxComponent2D>();
			box.halfExtent.x = wallHalfExtent.x;
			box.halfExtent.y = 1.0f;

			auto* floorMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Floor Mesh", floor->GetID());
			floorMesh->SetScale({ wallHalfExtent.x, 1.0f, 1.0f });
			floorMesh->SetMeshID(cubeMeshID);
			floorMesh->SetMaterialID(defaultMaterialInstID);
		}

		// Left Wall Node
		{
			auto* leftWall = sceneGraph->AddNode<TransformNode3D>("Left Wall");
			leftWall->SetPosition({ -wallHalfExtent.x, 0.0f, 0.0f });

			auto& rb = leftWall->AddComponent<physics::RigidbodyComponent2D>();
			auto& box = leftWall->AddComponent<physics::BoxComponent2D>();
			box.halfExtent.x = 1.0f;
			box.halfExtent.y = wallHalfExtent.y;

			auto* wallLeftMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Left Wall Mesh", leftWall->GetID());
			wallLeftMesh->SetScale({ 1.0f, wallHalfExtent.y, 1.0f });
			wallLeftMesh->SetMeshID(cubeMeshID);
			wallLeftMesh->SetMaterialID(defaultMaterialInstID);
		}

		// Right Wall Node
		{
			auto* rightWall = sceneGraph->AddNode<TransformNode3D>("Right Wall");
			rightWall->SetPosition({ wallHalfExtent.x, 0.0f, 0.0f });

			auto& rb = rightWall->AddComponent<physics::RigidbodyComponent2D>();
			auto& box = rightWall->AddComponent<physics::BoxComponent2D>();
			box.halfExtent.x = 1.0f;
			box.halfExtent.y = wallHalfExtent.y;

			auto* wallLeftMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Right Wall Mesh", rightWall->GetID());
			wallLeftMesh->SetScale({ 1.0f, wallHalfExtent.y, 1.0f });
			wallLeftMesh->SetMeshID(cubeMeshID);
			wallLeftMesh->SetMaterialID(defaultMaterialInstID);
		}

		// Ceiling Node
		{
			auto* ceiling = sceneGraph->AddNode<TransformNode3D>("Floor");
			ceiling->SetPosition({ 0.0f, wallHalfExtent.y, 0.0f });

			auto& rb = ceiling->AddComponent<physics::RigidbodyComponent2D>();
			auto& box = ceiling->AddComponent<physics::BoxComponent2D>();
			box.halfExtent.x = wallHalfExtent.x;
			box.halfExtent.y = 1.0f;

			auto* floorMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Ceiling Mesh", ceiling->GetID());
			floorMesh->SetScale({ wallHalfExtent.x, 1.0f, 1.0f });
			floorMesh->SetMeshID(cubeMeshID);
			floorMesh->SetMaterialID(defaultMaterialInstID);
		}

		// Box Nodes
		{
			constexpr int numBodies = 5000;

			const Vector2f bodyHalfExtent = { wallHalfExtent.x - 5.f, wallHalfExtent.y - 5.f };
			double velocityMax = 50.0;

			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_real_distribution posXDist(-bodyHalfExtent.x, bodyHalfExtent.x);
			std::uniform_real_distribution posYDist(-bodyHalfExtent.y, bodyHalfExtent.y);
			std::uniform_real_distribution velDist(-velocityMax, velocityMax);

			for (int i = 0; i < numBodies; ++i)
			{
				auto* body = sceneGraph->AddNode<TransformNode3D>("Body #" + std::to_string(i));
				body->SetPosition({ posXDist(mt), posYDist(mt), 0.0f });

				auto& rb = body->AddComponent<physics::RigidbodyComponent2D>();
				rb.bodyType = physics::BodyType::Dynamic;
				rb.friction = 0.0f;

				auto& velocity = body->AddComponent<physics::VelocityComponent3D>();
				velocity.linear.x = velDist(mt);
				velocity.linear.y = velDist(mt);

				auto& box = body->AddComponent<physics::BoxComponent2D>();
				box.halfExtent.x = 1.0f;
				box.halfExtent.y = 1.0f;

				auto* bodyMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("BodyMesh", body->GetID());
				bodyMesh->SetMeshID(cubeMeshID);
				bodyMesh->SetMaterialID(defaultMaterialInstID);
			}
		}
	}

	void SetupDefaultScene3D(const std::shared_ptr<Engine>& engine)
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadingDefaultMaterialInstPath)->GetID();
		UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadinChaletMaterialInstPath)->GetID();

		auto registry = engine->GetSubsystem<ecs::EnTTSubsystem>()->GetRegistry();
		const auto sceneGraph = engine->GetSubsystem<scene::SceneGraphSubsystem>();

		auto* houseNode = sceneGraph->AddNode<rendering::StaticMeshNode3D>("House");
		houseNode->SetPosition({ 2.0f, 0.0f, 0.0f });
		houseNode->SetMeshID(chaletMeshID);
		houseNode->SetMaterialID(defaultMaterialInstID);

		/*auto sphere = scene_graph->add_node<rendering::MeshNode>();
		sphere.SetName("Sphere");
		sphere.SetPosition({ -1.0f, -0.0f, 0.0f });
		sphere.SetMeshID(meshId2);
		sphere.SetMaterialID(materialInstId1);*/

		auto* cube1 = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Cube 1");
		cube1->SetPosition({ 0.0f });
		cube1->SetMeshID(cubeMeshID);
		cube1->SetMaterialID(defaultMaterialInstID);

		auto* cube2 = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Cube 2");
		cube2->SetPosition({ -1.75f, -5.0f, 0.0f });
		cube2->SetMeshID(cubeMeshID);
		cube2->SetMaterialID(defaultMaterialInstID);

		auto* plane = sceneGraph->AddNode<rendering::StaticMeshNode3D>("Plane");
		plane->SetPosition({ 0.0f, -10.0f, 0.0f });
		plane->SetScale({ 50.0f, 1.0f, 50.0f });
		plane->SetMeshID(cubeMeshID);
		plane->SetMaterialID(defaultMaterialInstID);

		auto* dirLight = sceneGraph->AddNode<rendering::DirectionalLightNode3D>("Directional Light");
		dirLight->SetPosition({ 0.0f, 10.0f, 0.0f });
		dirLight->SetColor({ .05f });
		dirLight->SetAmbientIntensity(.0f);
		dirLight->AddComponent<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(dirLight->GetEntity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
			shadow.biasMin = 0.3f;
			shadow.biasMax = 0.5f;
		});

		UpdateTransformOrientation(dirLight->Transform(), { 0.0f, -90.0f, 0.0f });

		auto* dirLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Directional Light Mesh", dirLight->GetID());
		dirLightMesh->SetScale({ 0.25f });
		dirLightMesh->SetMeshID(cubeMeshID);
		dirLightMesh->SetMaterialID(defaultMaterialInstID);

		auto* spotLight = sceneGraph->AddNode<rendering::SpotLightNode3D>("Spot Light");
		spotLight->SetPosition({ -10.0f, 5.0f, 0.0f });
		spotLight->SetColor({ 0.5f, 0.5f, 1.0f });
		spotLight->SetAmbientIntensity(0.f);
		spotLight->AddComponent<rendering::ShadowCasterComponent3D>();
		registry->patch<rendering::ShadowCasterComponent3D>(spotLight->GetEntity(), [&](auto& shadow)
		{
			shadow.width = 8192;
			shadow.height = 8192;
		});

		auto* spotLightMesh = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Spot Light Mesh", spotLight->GetID());
		spotLightMesh->SetScale({ 0.25f });
		spotLightMesh->SetMeshID(cubeMeshID);
		spotLightMesh->SetMaterialID(defaultMaterialInstID);

		auto* spotLight2 = sceneGraph->AddNode<rendering::SpotLightNode3D>("Spot Light 2");
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

		auto* spotLightMesh2 = sceneGraph->AddChildNode<rendering::StaticMeshNode3D>("Spot Light 2 Mesh", spotLight2->GetID());
		spotLightMesh2->SetScale({ 0.25f });
		spotLightMesh2->SetMeshID(cubeMeshID);
		spotLightMesh2->SetMaterialID(defaultMaterialInstID);
	}

	void SetupDefaultPhysicsScene3D(const std::shared_ptr<Engine>& engine)
	{
		// Get assets
		fs::path contentRootPath = assets::AssetRegistry::Get()->GetContentRoot();

		const UUID chaletMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gChaletMeshPath)->GetID();
		const UUID cubeMeshID = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(gCubeMeshPath)->GetID();

		const UUID chaletTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gChaletTexturePath)->GetID();
		const UUID cubeTexID = assets::AssetRegistry::Get()->GetAsset<assets::TextureAsset>(gCubeTexturePath)->GetID();

		UUID defaultMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadingDefaultMaterialInstPath)->GetID();
		UUID chaletMaterialInstID = assets::AssetRegistry::Get()->GetAsset<assets::MaterialInstanceAsset>(
			gForwardShadinChaletMaterialInstPath)->GetID();

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
