#include "Rendering/BGFX/BGFXRenderSystem.hpp"

#include "bgfx/bgfx.h"
#include "bx/math.h"

#include "Engine/Engine.hpp"
#include "Window/WindowSubsystem.hpp"

#include "Assets/AssetRegistry.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "MathHelpers.h"
#include "Engine/SignalSubsystem.hpp"
#include "Assets/TextureAsset.h"
#include "Input/InputSubsystem.h"

#include <vector>

namespace puffin::rendering
{
	void BGFXRenderSystem::Init()
	{
        InitBGFX();

        InitStaticCubeData();

        InitMeshProgram();

        InitTexSamplers();

        InitCamUniforms();

        InitLightUniforms();

        // Connect Signals
        const auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

        signalSubsystem->connect<Input::InputEvent>(
			[&](const Input::InputEvent& inputEvent)
			{
				shared_from_this()->OnInputEvent(inputEvent);
			}
        );

        // Register Components
        mWorld->RegisterComponent<CameraMatComponent>();
        mWorld->AddComponentDependencies<CameraComponent, CameraMatComponent>();
	}

	void BGFXRenderSystem::Setup()
	{
		InitEditorCamera();

        InitComponents();
	}

	void BGFXRenderSystem::Render()
	{
		ProcessEvents();

        UpdateEditorCamera();

		UpdateComponents();

        Draw();
	}

	void BGFXRenderSystem::Stop()
	{
		CleanupComponents();
	}

	void BGFXRenderSystem::Cleanup()
	{
        m_deletionQueue.Flush();

		bgfx::shutdown();
	}

	void BGFXRenderSystem::OnInputEvent(const Input::InputEvent& inputEvent)
	{
        m_inputEvents.Push(inputEvent);
	}

	void BGFXRenderSystem::InitBGFX()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = mEngine->getSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FrameBufferResizeCallback);

#if PFN_PLATFORM_WIN32

        pd.ndt = NULL;
        pd.nwh = glfwGetWin32Window(window);

#else

        pd.ndt = glfwGetX11Display();
        pd.nwh = (void*)glfwGetX11Window(window);

        // Set Wayland instead of X11
        // pd.ndt = glfwGetWaylandDisplay();

#endif

        bgfx::Init bgfxInit;

        // Set Renderer API to Vulkan
        bgfxInit.type = bgfx::RendererType::Vulkan;

        glfwGetWindowSize(window, &m_windowWidth, &m_windowHeight);

        bgfxInit.resolution.width = static_cast<uint32_t>(m_windowWidth);
        bgfxInit.resolution.height = static_cast<uint32_t>(m_windowHeight);
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);

        bgfx::setDebug(BGFX_DEBUG_NONE);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00B5E2FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));

        // Check supported features
        const bgfx::Caps* caps = bgfx::getCaps();

        m_supportsInstancing = 0 != (BGFX_CAPS_INSTANCING & caps->supported);
	}

	void BGFXRenderSystem::InitStaticCubeData()
	{
        // Create Static Vertex Buffer
        m_cubeMeshData.vertexBufferHandle = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), s_layoutVertexPC32);

        // Create Static Index Buffer
        m_cubeMeshData.indexBufferHandle = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));

        bgfx::ShaderHandle cubeVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\vs_cubes.bin");
        bgfx::ShaderHandle cubeFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\fs_cubes.bin");
        m_cubeProgram = bgfx::createProgram(cubeVSH, cubeFSH, true);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_cubeMeshData.indexBufferHandle);
			bgfx::destroy(m_cubeMeshData.vertexBufferHandle);

			bgfx::destroy(m_cubeProgram);
        });
	}

	void BGFXRenderSystem::InitMeshProgram()
	{
        bgfx::ShaderHandle meshVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading.bin");
        bgfx::ShaderHandle meshFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading.bin");

        m_meshProgram = bgfx::createProgram(meshVSH, meshFSH, true);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_meshProgram);
        });
	}

	void BGFXRenderSystem::InitMeshInstancedProgram()
	{
        bgfx::ShaderHandle meshVSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading_instanced.bin");
        bgfx::ShaderHandle meshFSH = LoadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading_instanced.bin");

        m_meshInstancedProgram = bgfx::createProgram(meshVSH, meshFSH, true);

        m_deletionQueue.PushFunction([=]()
            {
                bgfx::destroy(m_meshInstancedProgram);
            });
	}

	void BGFXRenderSystem::InitTexSamplers()
	{
        m_texAlbedoSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
        m_texNormalSampler = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_texAlbedoSampler);
			bgfx::destroy(m_texNormalSampler);
        });
	}

	void BGFXRenderSystem::InitCamUniforms()
	{
		m_camPosHandle = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4, 1);

        m_deletionQueue.PushFunction([=]()
        {
        	bgfx::destroy(m_camPosHandle);
        });
	}

	void BGFXRenderSystem::InitLightUniforms()
	{
        m_lightUniformHandles.position = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.direction = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.color = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.ambientSpecular = bgfx::createUniform("u_lightAmbientSpecular", bgfx::UniformType::Vec4, G_MAX_LIGHTS);
        m_lightUniformHandles.attenuation = bgfx::createUniform("u_lightAttenuation", bgfx::UniformType::Vec4, G_MAX_LIGHTS);

        m_lightUniformHandles.index = bgfx::createUniform("u_lightIndex", bgfx::UniformType::Vec4, 1);

        m_deletionQueue.PushFunction([=]()
        {
            bgfx::destroy(m_lightUniformHandles.position);
	        bgfx::destroy(m_lightUniformHandles.direction);
	        bgfx::destroy(m_lightUniformHandles.color);
	        bgfx::destroy(m_lightUniformHandles.ambientSpecular);
            bgfx::destroy(m_lightUniformHandles.attenuation);
            bgfx::destroy(m_lightUniformHandles.index);
        });
	}

	void BGFXRenderSystem::ProcessEvents()
	{
        Input::InputEvent inputEvent;
		while(m_inputEvents.Pop(inputEvent))
		{
            if (inputEvent.actionName == "CamMoveLeft")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveLeft = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveLeft = false;
                }
            }

            if (inputEvent.actionName == "CamMoveRight")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveRight = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveRight = false;
                }
            }

            if (inputEvent.actionName == "CamMoveForward")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveForward = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveForward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveBackward")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveBackward = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveBackward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveUp")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveUp = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveUp = false;
                }
            }

            if (inputEvent.actionName == "CamMoveDown")
            {
                if (inputEvent.actionState == puffin::Input::KeyState::PRESSED)
                {
                    m_moveDown = true;
                }
                else if (inputEvent.actionState == puffin::Input::KeyState::RELEASED)
                {
                    m_moveDown = false;
                }
            }
		}
	}

	void BGFXRenderSystem::InitComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
            if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
            {
                InitMeshComponent(entity);

                entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
            }
        }

        PackedVector<ECS::EntityPtr> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(mWorld, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
				UpdateCameraComponent(entity);

                entity->SetComponentFlag<CameraComponent, FlagDirty>(false);
            }
        }
	}

	void BGFXRenderSystem::UpdateComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
			if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
			{
                CleanupMeshComponent(entity);
				InitMeshComponent(entity);

                entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
			}

            if (entity->GetComponentFlag<MeshComponent, FlagDeleted>())
            {
				CleanupMeshComponent(entity);

                entity->RemoveComponent<MeshComponent>();
            }
        }

        PackedVector<ECS::EntityPtr> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(mWorld, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
                UpdateCameraComponent(entity);

                entity->SetComponentFlag<CameraComponent, FlagDirty>(false);
            }

            if (entity->GetComponentFlag<CameraComponent, FlagDeleted>())
            {
                entity->RemoveComponent<CameraComponent>();
                entity->RemoveComponent<CameraMatComponent>();
            }
        }

        PackedVector<ECS::EntityPtr> lightEntities;
        ECS::GetEntities<TransformComponent, LightComponent>(mWorld, lightEntities);
        for (const auto& entity : lightEntities)
        {
	        
        }
	}

	void BGFXRenderSystem::CleanupComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
            CleanupMeshComponent(entity);
        }
	}

	void BGFXRenderSystem::Draw()
	{
        // Resize view and back-buffer
        if (m_windowResized)
        {
            bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));
            bgfx::reset(static_cast<uint16_t>(m_windowWidth), static_cast<uint16_t>(m_windowHeight));

            m_windowResized = false;
        }

        // Dummy draw call to make sure view 0 is cleared
        bgfx::touch(0);

        DrawScene();

        // Advance to next frame
        bgfx::frame();

        m_frameCounter++;
	}

	void BGFXRenderSystem::DrawScene()
	{
        // Set Camera Matrices
        bgfx::setViewTransform(0, m_editorCamMats.view, m_editorCamMats.proj);

        // Set Light Uniforms
        SetupLightUniformsForDraw();

        // Generate Mesh Batches (Batches of entities which share same mesh and material) for this frame
        std::vector<MeshDrawBatch> batches;
        BuildBatches(batches);

        // Render each batch
        for (const auto& drawBatch : batches)
        {
            // Set Vertex/Index Buffers
            bgfx::setVertexBuffer(0, drawBatch.meshData.vertexBufferHandle);
            bgfx::setIndexBuffer(drawBatch.meshData.indexBufferHandle);

            // Set Material/Textures
            int tidx = 0;
            for (const auto& id : drawBatch.matData.texIDs)
            {
                bgfx::setTexture(tidx, m_texAlbedoSampler, m_texData[id].handle);

                tidx++;
            }

            // Render using instanced rendering if supported/enableded, and there is more than one entity in batch
            if (m_useInstancing && m_supportsInstancing && drawBatch.entities.size() > 1)
            {
	            DrawMeshBatchInstanced(drawBatch);
            }
            // Else use normal rendering path
            else
            {
	            DrawMeshBatch(drawBatch);
            }

            // Discard Vertex/Index/Buffers/Textures after rendering all entities in batch
            bgfx::discard(BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_BINDINGS);
        }

        bgfx::discard(BGFX_DISCARD_ALL);
	}

	void BGFXRenderSystem::BuildBatches(std::vector<MeshDrawBatch>& batches)
	{
		batches.clear();

        batches.reserve(m_meshData.Size() * m_matData.Size());

        std::unordered_map<MeshMatPair, std::set<ECS::EntityID>> meshMatMap;

        // Generate set of entities for each unique mesh/material pair
        for (const auto& meshData : m_meshData)
        {
	        const std::set<ECS::EntityID>& meshEntities = m_meshSets[meshData.assetID];

            for (const ECS::EntityID& entity : meshEntities)
            {
                auto& mesh = mWorld->GetComponent<MeshComponent>(entity);

                MeshMatPair pair(mesh.textureAssetId, meshData.assetID);

                if (meshMatMap.count(pair) == 0)
                {
                    meshMatMap.emplace(pair, std::set<ECS::EntityID>());
                }

                meshMatMap[pair].insert(entity);
            }
        }

        // Generate batch for each unique mesh/material pair
        for (const auto& [fst, snd] : meshMatMap)
        {
			UUID matID = fst.first;
            UUID meshID = fst.second;

	        MeshDrawBatch batch;
            batch.meshData = m_meshData[meshID];
            batch.matData = m_matData[matID];
            batch.entities = snd;

            batches.push_back(batch);
        }
	}

	void BGFXRenderSystem::DrawMeshBatch(const MeshDrawBatch& meshDrawBatch)
	{
        for (const auto& entity : meshDrawBatch.entities)
        {
            const auto& transform = mWorld->GetComponent<TransformComponent>(entity);

            // Setup Transform
            float model[16];

            BuildModelTransform(transform, model);

            bgfx::setTransform(model);

            // Submit Program
            bgfx::submit(0, meshDrawBatch.matData.programHandle, 0, BGFX_DISCARD_TRANSFORM | BGFX_DISCARD_STATE);
        }
	}

	void BGFXRenderSystem::DrawMeshBatchInstanced(const MeshDrawBatch& meshDrawBatch)
	{
        // Setup stride for instanced rendering
        const uint16_t instanceStride = 80; // 64 Bytes for 4x4 matrix, 16 Bytes for Instance Index

        const uint32_t numEntities = meshDrawBatch.entities.size();
        const uint32_t numInstances = bgfx::getAvailInstanceDataBuffer(numEntities, instanceStride);

        bgfx::InstanceDataBuffer idb = {};
        bgfx::allocInstanceDataBuffer(&idb, numInstances, instanceStride);

        uint8_t* data = idb.data;

        int idx = 0;
        for (const auto& entity : meshDrawBatch.entities)
        {
            const auto& transform = mWorld->GetComponent<TransformComponent>(entity);

            // Setup Transform
            float* mtx = (float*)data;

            BuildModelTransform(transform, mtx);

            float* index = (float*)&data[64];
            index[0] = static_cast<float>(idx);
            index[1] = 0.0f;
            index[2] = 0.0f;
            index[3] = 0.0f;

            data += instanceStride;
            idx++;
        }

        bgfx::setInstanceDataBuffer(&idb, 0, numInstances);

        bgfx::setState(BGFX_STATE_DEFAULT);

        // Submit Program
        bgfx::submit(0, meshDrawBatch.matData.programHandle, 0, BGFX_DISCARD_TRANSFORM | BGFX_DISCARD_STATE | BGFX_DISCARD_INSTANCE_DATA);
    }

	void BGFXRenderSystem::InitMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        // Init Mesh
        if (!m_meshData.Contains(mesh.meshAssetId))
        {
			MeshData meshData;

			LoadAndInitMesh(mesh.meshAssetId, meshData);

            m_meshData.Insert(mesh.meshAssetId, meshData);
            m_meshSets.emplace(mesh.meshAssetId, std::set<ECS::EntityID>());
        }

        m_meshSets[mesh.meshAssetId].insert(entity->ID());

        // Init Textures
        if (!m_texData.Contains(mesh.textureAssetId))
        {
			TextureData texData;

            LoadAndInitTexture(mesh.textureAssetId, texData);

            m_texData.Insert(mesh.textureAssetId, texData);
            m_texSets.emplace(mesh.textureAssetId, std::set<ECS::EntityID>());
        }

        m_texSets[mesh.textureAssetId].insert(entity->ID());

        // Init Materials (Currently using default mesh program, will add proper material system later)
        if (!m_matData.Contains(mesh.textureAssetId))
        {
	        MaterialData matData;

            matData.assetID = mesh.textureAssetId;

            if (m_useInstancing && m_supportsInstancing)
            {
                matData.programHandle = m_meshInstancedProgram;
            }
            else
            {
                matData.programHandle = m_meshProgram;
            }

            matData.texIDs.push_back(mesh.textureAssetId);

            m_matData.Insert(mesh.textureAssetId, matData);
            m_matSets.emplace(mesh.textureAssetId, std::set<ECS::EntityID>());
        }

        m_matSets[mesh.textureAssetId].insert(entity->ID());
	}

	void BGFXRenderSystem::CleanupMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        // Cleanup Mesh Data
        m_meshSets[mesh.meshAssetId].erase(entity->ID());

        if (m_meshSets[mesh.meshAssetId].empty())
        {
            bgfx::destroy(m_meshData[mesh.meshAssetId].vertexBufferHandle);
            bgfx::destroy(m_meshData[mesh.meshAssetId].indexBufferHandle);

            m_meshData.Erase(mesh.meshAssetId);
            m_meshSets.erase(mesh.meshAssetId);
        }

        // Cleanup Material Data
        m_matSets[mesh.textureAssetId].erase(entity->ID());

        if (m_matSets[mesh.textureAssetId].empty())
        {
			m_matData.Erase(mesh.textureAssetId);
            m_matSets.erase(mesh.textureAssetId);
        }

        // Cleanup Texture Data
        m_texSets[mesh.textureAssetId].erase(entity->ID());

        if (m_texSets[mesh.textureAssetId].empty())
        {
			bgfx::destroy(m_texData[mesh.textureAssetId].handle);

            m_texData.Erase(mesh.textureAssetId);
            m_texSets.erase(mesh.textureAssetId);
        }
	}

	void BGFXRenderSystem::InitEditorCamera()
	{
        m_editorCam.position = {0.0f, 0.0f, 15.0f};
        m_editorCam.aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
        m_editorCam.lookAt = m_editorCam.position + m_editorCam.direction;

        bx::mtxLookAt(m_editorCamMats.view, static_cast<bx::Vec3>(m_editorCam.position), static_cast<bx::Vec3>(m_editorCam.lookAt), { 0, 1, 0 }, bx::Handedness::Right);
        bx::mtxProj(m_editorCamMats.proj, m_editorCam.fovY, m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
	}

	void BGFXRenderSystem::UpdateEditorCamera()
    {
        const auto inputSubsystem = mEngine->getSubsystem<Input::InputSubsystem>();

        if (inputSubsystem->IsCursorLocked())
        {
            // Camera Movement
            if (m_moveLeft && !m_moveRight)
            {
                m_editorCam.position += m_editorCam.right * m_editorCam.speed * mEngine->deltaTime();
            }

            if (m_moveRight && !m_moveLeft)
            {
                m_editorCam.position -= m_editorCam.right * m_editorCam.speed * mEngine->deltaTime();
            }

            if (m_moveForward && !m_moveBackward)
            {
                m_editorCam.position += m_editorCam.direction * m_editorCam.speed * mEngine->deltaTime();
            }

            if (m_moveBackward && !m_moveForward)
            {
                m_editorCam.position -= m_editorCam.direction * m_editorCam.speed * mEngine->deltaTime();
            }

            if (m_moveUp && !m_moveDown)
            {
                m_editorCam.position += m_editorCam.up * m_editorCam.speed * mEngine->deltaTime();
            }

            if (m_moveDown && !m_moveUp)
            {
                m_editorCam.position -= m_editorCam.up * m_editorCam.speed * mEngine->deltaTime();
            }

            // Mouse Rotation
            m_editorCam.yaw += inputSubsystem->GetMouseXOffset();
            m_editorCam.pitch -= inputSubsystem->GetMouseYOffset();

            if (m_editorCam.pitch > 89.0f)
                m_editorCam.pitch = 89.0f;

            if (m_editorCam.pitch < -89.0f)
                m_editorCam.pitch = -89.0f;

            // Calculate Direction vector from yaw and pitch of camera
            m_editorCam.direction.x = cos(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));
            m_editorCam.direction.y = sin(Maths::DegreesToRadians(m_editorCam.pitch));
            m_editorCam.direction.z = sin(Maths::DegreesToRadians(m_editorCam.yaw)) * cos(Maths::DegreesToRadians(m_editorCam.pitch));

            m_editorCam.direction.Normalise();
        }

        // Calculate Right, Up and LookAt vectors
        m_editorCam.right = m_editorCam.up.Cross(m_editorCam.direction).Normalised();
        m_editorCam.lookAt = m_editorCam.position + m_editorCam.direction;

        if (m_windowResized)
        {
            m_editorCam.aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
        }

        bx::mtxLookAt(m_editorCamMats.view, static_cast<bx::Vec3>(m_editorCam.position), static_cast<bx::Vec3>(m_editorCam.lookAt), {0, 1, 0}, bx::Handedness::Right);
        bx::mtxProj(m_editorCamMats.proj, m_editorCam.fovY, m_editorCam.aspect, m_editorCam.zNear, m_editorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    }

	void BGFXRenderSystem::UpdateCameraComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& transform = entity->GetComponent<TransformComponent>();
        auto& cam = entity->GetComponent<CameraComponent>();
        auto& camMats = entity->GetComponent<CameraMatComponent>();

        // Calculate Right, Up and LookAt vectors
        cam.right = cam.up.Cross(transform.rotation.GetXYZ()).Normalised();
        cam.lookAt = transform.position + transform.rotation.GetXYZ();

        bx::mtxLookAt(camMats.view, static_cast<bx::Vec3>(transform.position), static_cast<bx::Vec3>(cam.lookAt), { 0, 1, 0 }, bx::Handedness::Right);

        // Recalculate camera perspective if fov has changed, store new fov in prevFov
        if (cam.fovY != cam.prevFovY)
        {
            cam.prevFovY = cam.fovY;

            bx::mtxProj(camMats.proj, cam.fovY, cam.aspect, cam.zNear, cam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
        }
	}

	void BGFXRenderSystem::LoadAndInitMesh(UUID meshID, MeshData& meshData)
	{
        const auto meshAsset = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshID));

        if (meshAsset && meshAsset->load())
        {
            meshData.assetID = meshID;
            meshData.numVertices = meshAsset->numVertices();
            meshData.numIndices = meshAsset->numIndices();

            meshData.vertexBufferHandle = InitVertexBuffer(meshAsset->vertices().data(), meshData.numVertices, s_layoutVertexPNTV32);
            meshData.indexBufferHandle = InitIndexBuffer(meshAsset->indices().data(), meshData.numIndices, true);

            meshAsset->unload();
        }
	}

	bgfx::VertexBufferHandle BGFXRenderSystem::InitVertexBuffer(const void* vertices, const uint32_t& numVertices,
		const bgfx::VertexLayout& layout)
	{
		// Allocate memory for vertex buffer
        const bgfx::Memory* mem = bgfx::alloc(numVertices * layout.getStride());

        // Copy vertices to allocated memory
        bx::memCopy(mem->data, vertices, mem->size);

        // Create vertex buffer
        return bgfx::createVertexBuffer(mem, layout);
	}

	bgfx::IndexBufferHandle BGFXRenderSystem::InitIndexBuffer(const void* indices, const uint32_t numIndices,
		bool use32BitIndices)
	{
        // Set indices size and flags based on whether 16 or 32 bit indices are used
		uint16_t indicesSize;
        uint16_t indexFlags = 0;

        if (use32BitIndices)
        {
	        indicesSize = sizeof(uint32_t);
            indexFlags = BGFX_BUFFER_INDEX32;
        }
        else
        {
            indicesSize = sizeof(uint16_t);
        }

        // Allocate memory for index buffer
        const bgfx::Memory* mem = bgfx::alloc(numIndices * indicesSize);

        // Copy indices to allocated memory
        bx::memCopy(mem->data, indices, mem->size);

        // Create index buffer
        return bgfx::createIndexBuffer(mem, indexFlags);
	}

    void BGFXRenderSystem::LoadAndInitTexture(UUID texID, TextureData& texData)
    {
        const auto texAsset = std::static_pointer_cast<assets::TextureAsset>(assets::AssetRegistry::get()->getAsset(texID));

        if (texAsset && texAsset->load())
        {
            texData.assetID = texID;

            uint32_t texSize = texAsset->textureSize();

            const bgfx::Memory* mem = bgfx::alloc(texSize);

            bx::memCopy(mem->data, texAsset->pixelData(), mem->size);

            texData.handle = bgfx::createTexture2D(texAsset->textureWidth(), texAsset->textureHeight(),
                false, 1, g_texFormatMap.at(texAsset->textureFormat()), 0, mem);

            texAsset->unload();
        }
    }

    void BGFXRenderSystem::SetupLightUniformsForDraw() const
    {
		// Setup Light Data
        float lightPos[G_MAX_LIGHTS][4];
        float lightDir[G_MAX_LIGHTS][4];
        float lightColor[G_MAX_LIGHTS][4];
        float lightAmbientSpec[G_MAX_LIGHTS][4];
        float lightAttenuation[G_MAX_LIGHTS][4];
        float lightIndex[4];

        int index = 0;

        PackedVector<ECS::EntityPtr> lightEntities;
        ECS::GetEntities<TransformComponent, LightComponent>(mWorld, lightEntities);

        std::vector<ECS::EntityPtr> lightEntitiesOrdered;

        // Sort Lights into DIRECTIONAL, POINT, SPOT Order
        lightEntitiesOrdered.reserve(lightEntities.Size());
        
        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

	        if (light.type == LightType::Directional)
	        {
                lightEntitiesOrdered.push_back(entity);
                index++;
	        }
        }

        lightIndex[0] = static_cast<float>(index);

        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

            if (light.type == LightType::Point)
            {
                lightEntitiesOrdered.push_back(entity);
                index++;
            }
        }

        lightIndex[1] = static_cast<float>(index);

        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= G_MAX_LIGHTS)
            {
                break;
            }

            const auto& light = entity->GetComponent<LightComponent>();

            if (light.type == LightType::Spot)
            {
                lightEntitiesOrdered.push_back(entity);
                index++;
            }
        }

        lightIndex[2] = static_cast<float>(index);

        index = 0;

        // Iterate through sorted 
        for (const auto& entity : lightEntitiesOrdered)
        {
            // Break out of loop when number of lights exceeds max
			if (index >= G_MAX_LIGHTS)
			{
				break;
			}

            const auto& transform = entity->GetComponent<TransformComponent>();
            const auto& light = entity->GetComponent<LightComponent>();

            lightPos[index][0] = transform.position.x;
            lightPos[index][1] = transform.position.y;
            lightPos[index][2] = transform.position.z;
            lightPos[index][3] = 0.0f;

            lightDir[index][0] = transform.rotation.x;
            lightDir[index][1] = transform.rotation.y;
            lightDir[index][2] = transform.rotation.z;
            lightDir[index][3] = 0.0f;

            lightColor[index][0] = light.color.x;
            lightColor[index][1] = light.color.y;
            lightColor[index][2] = light.color.z;
            lightColor[index][3] = 0.0f;

            lightAmbientSpec[index][0] = light.ambientIntensity;
            lightAmbientSpec[index][1] = light.specularIntensity;
            lightAmbientSpec[index][2] = static_cast<float>(light.specularExponent);
            lightAmbientSpec[index][3] = 0.0f;

            lightAttenuation[index][0] = light.constantAttenuation;
            lightAttenuation[index][1] = light.linearAttenuation;
            lightAttenuation[index][2] = light.quadraticAttenuation;
            lightAttenuation[index][3] = 0.0f;

            index++;
        }

        // Set Light Uniforms
        
		bgfx::setUniform(m_lightUniformHandles.position, lightPos, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.direction, lightDir, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.color, lightColor, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.ambientSpecular, lightAmbientSpec, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.attenuation, lightAttenuation, G_MAX_LIGHTS);
        bgfx::setUniform(m_lightUniformHandles.index, lightIndex, 1);
    }

    void BGFXRenderSystem::BuildModelTransform(const TransformComponent& transform, float* model)
	{
        // Identity
		bx::mtxIdentity(model);

        // Scale
        bx::mtxScale(model, transform.scale.x, transform.scale.y, transform.scale.z);

        Vector3f euler = transform.rotation.EulerAnglesRad();

        // Rotation
        bx::mtxRotateZ(model, euler.z);
        bx::mtxRotateX(model, euler.x);
        bx::mtxRotateY(model, euler.y);

        // Translation
        bx::mtxTranslate(model, static_cast<float>(transform.position.x), static_cast<float>(transform.position.y), static_cast<float>(transform.position.z));
	}
}
