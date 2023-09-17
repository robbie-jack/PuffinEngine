#include "Rendering\BGFX\BGFXRenderSystem.h"

#include "bgfx/bgfx.h"
#include "bx/math.h"

#include "Engine\Engine.h"
#include "Window\WindowSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/CameraComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "MathHelpers.h"
#include "Engine\SignalSubsystem.h"
#include "Assets/TextureAsset.h"
#include "Input/InputSubsystem.h"

#include <vector>

namespace puffin::rendering
{
	void BGFXRenderSystem::init()
	{
        initBGFX();

        initStaticCubeData();

        initMeshProgram();

        initTexSamplers();

        initCamUniforms();

        initLightUniforms();

        // Connect Signals
        const auto signalSubsystem = mEngine->getSubsystem<core::SignalSubsystem>();

        signalSubsystem->connect<input::InputEvent>(
			[&](const input::InputEvent& inputEvent)
			{
				shared_from_this()->onInputEvent(inputEvent);
			}
        );

        // Register Components
        mWorld->RegisterComponent<CameraMatComponent>();
        mWorld->AddComponentDependencies<CameraComponent, CameraMatComponent>();
	}

	void BGFXRenderSystem::setup()
	{
		initEditorCamera();

        initComponents();
	}

	void BGFXRenderSystem::render()
	{
		processEvents();

        updateEditorCamera();

		updateComponents();

        draw();
	}

	void BGFXRenderSystem::stop()
	{
		cleanupComponents();
	}

	void BGFXRenderSystem::cleanup()
	{
        mDeletionQueue.flush();

		bgfx::shutdown();
	}

	void BGFXRenderSystem::onInputEvent(const input::InputEvent& inputEvent)
	{
        mInputEvents.push(inputEvent);
	}

	void BGFXRenderSystem::initBGFX()
	{
        bgfx::PlatformData pd;
        GLFWwindow* window = mEngine->getSubsystem<Window::WindowSubsystem>()->GetPrimaryWindow();

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);

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

        glfwGetWindowSize(window, &mWindowWidth, &mWindowHeight);

        bgfxInit.resolution.width = static_cast<uint32_t>(mWindowWidth);
        bgfxInit.resolution.height = static_cast<uint32_t>(mWindowHeight);
        bgfxInit.resolution.reset = BGFX_RESET_VSYNC;
        bgfxInit.platformData = pd;
        bgfx::init(bgfxInit);

        bgfx::setDebug(BGFX_DEBUG_NONE);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x00B5E2FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(mWindowWidth), static_cast<uint16_t>(mWindowHeight));

        // Check supported features
        const bgfx::Caps* caps = bgfx::getCaps();

        mSupportsInstancing = 0 != (BGFX_CAPS_INSTANCING & caps->supported);
	}

	void BGFXRenderSystem::initStaticCubeData()
	{
        // Create Static Vertex Buffer
        mCubeMeshData.vertexBufferHandle = bgfx::createVertexBuffer(bgfx::makeRef(gCubeVertices, sizeof(gCubeVertices)), gLayoutVertexPC32);

        // Create Static Index Buffer
        mCubeMeshData.indexBufferHandle = bgfx::createIndexBuffer(bgfx::makeRef(gCubeTriList, sizeof(gCubeTriList)));

        const bgfx::ShaderHandle cubeVsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\vs_cubes.bin");
        const bgfx::ShaderHandle cubeFsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\fs_cubes.bin");
        mCubeProgram = bgfx::createProgram(cubeVsh, cubeFsh, true);

        mDeletionQueue.pushFunction([=]()
        {
        	bgfx::destroy(mCubeMeshData.indexBufferHandle);
			bgfx::destroy(mCubeMeshData.vertexBufferHandle);

			bgfx::destroy(mCubeProgram);
        });
	}

	void BGFXRenderSystem::initMeshProgram()
	{
		const bgfx::ShaderHandle meshVsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading.bin");
		const bgfx::ShaderHandle meshFsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading.bin");

        mMeshProgram = bgfx::createProgram(meshVsh, meshFsh, true);

        mDeletionQueue.pushFunction([=]()
        {
        	bgfx::destroy(mMeshProgram);
        });
	}

	void BGFXRenderSystem::initMeshInstancedProgram()
	{
		const bgfx::ShaderHandle meshVsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\vs_forward_shading_instanced.bin");
		const bgfx::ShaderHandle meshFsh = loadShader("C:\\Projects\\PuffinEngine\\bin\\bgfx\\spirv\\forward_shading\\fs_forward_shading_instanced.bin");

        mMeshInstancedProgram = bgfx::createProgram(meshVsh, meshFsh, true);

        mDeletionQueue.pushFunction([=]()
            {
                bgfx::destroy(mMeshInstancedProgram);
            });
	}

	void BGFXRenderSystem::initTexSamplers()
	{
        mTexAlbedoSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
        mTexNormalSampler = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);

        mDeletionQueue.pushFunction([=]()
        {
        	bgfx::destroy(mTexAlbedoSampler);
			bgfx::destroy(mTexNormalSampler);
        });
	}

	void BGFXRenderSystem::initCamUniforms()
	{
		mCamPosHandle = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4, 1);

        mDeletionQueue.pushFunction([=]()
        {
        	bgfx::destroy(mCamPosHandle);
        });
	}

	void BGFXRenderSystem::initLightUniforms()
	{
        mLightUniformHandles.position = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, gMaxLightsBGFX);
        mLightUniformHandles.direction = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4, gMaxLightsBGFX);
        mLightUniformHandles.color = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, gMaxLightsBGFX);
        mLightUniformHandles.ambientSpecular = bgfx::createUniform("u_lightAmbientSpecular", bgfx::UniformType::Vec4, gMaxLightsBGFX);
        mLightUniformHandles.attenuation = bgfx::createUniform("u_lightAttenuation", bgfx::UniformType::Vec4, gMaxLightsBGFX);

        mLightUniformHandles.index = bgfx::createUniform("u_lightIndex", bgfx::UniformType::Vec4, 1);

        mDeletionQueue.pushFunction([=]()
        {
            bgfx::destroy(mLightUniformHandles.position);
	        bgfx::destroy(mLightUniformHandles.direction);
	        bgfx::destroy(mLightUniformHandles.color);
	        bgfx::destroy(mLightUniformHandles.ambientSpecular);
            bgfx::destroy(mLightUniformHandles.attenuation);
            bgfx::destroy(mLightUniformHandles.index);
        });
	}

	void BGFXRenderSystem::processEvents()
	{
        input::InputEvent inputEvent;
		while(mInputEvents.pop(inputEvent))
		{
            if (inputEvent.actionName == "CamMoveLeft")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveLeft = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveLeft = false;
                }
            }

            if (inputEvent.actionName == "CamMoveRight")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveRight = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveRight = false;
                }
            }

            if (inputEvent.actionName == "CamMoveForward")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveForward = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveForward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveBackward")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveBackward = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveBackward = false;
                }
            }

            if (inputEvent.actionName == "CamMoveUp")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveUp = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveUp = false;
                }
            }

            if (inputEvent.actionName == "CamMoveDown")
            {
                if (inputEvent.actionState == puffin::input::KeyState::Pressed)
                {
                    mMoveDown = true;
                }
                else if (inputEvent.actionState == puffin::input::KeyState::Released)
                {
                    mMoveDown = false;
                }
            }
		}
	}

	void BGFXRenderSystem::initComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
            if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
            {
                initMeshComponent(entity);

                entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
            }
        }

        PackedVector<ECS::EntityPtr> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(mWorld, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
				updateCameraComponent(entity);

                entity->SetComponentFlag<CameraComponent, FlagDirty>(false);
            }
        }
	}

	void BGFXRenderSystem::updateComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
			if (entity->GetComponentFlag<MeshComponent, FlagDirty>())
			{
                cleanupMeshComponent(entity);
				initMeshComponent(entity);

                entity->SetComponentFlag<MeshComponent, FlagDirty>(false);
			}

            if (entity->GetComponentFlag<MeshComponent, FlagDeleted>())
            {
				cleanupMeshComponent(entity);

                entity->RemoveComponent<MeshComponent>();
            }
        }

        PackedVector<ECS::EntityPtr> cameraEntities;
        ECS::GetEntities<TransformComponent, CameraComponent>(mWorld, cameraEntities);
        for (const auto& entity : cameraEntities)
        {
            if (entity->GetComponentFlag<CameraComponent, FlagDirty>())
            {
                updateCameraComponent(entity);

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

	void BGFXRenderSystem::cleanupComponents()
	{
        PackedVector<ECS::EntityPtr> meshEntities;
        ECS::GetEntities<TransformComponent, MeshComponent>(mWorld, meshEntities);
        for (const auto& entity : meshEntities)
        {
            cleanupMeshComponent(entity);
        }
	}

	void BGFXRenderSystem::draw()
	{
        // Resize view and back-buffer
        if (mWindowResized)
        {
            bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(mWindowWidth), static_cast<uint16_t>(mWindowHeight));
            bgfx::reset(static_cast<uint16_t>(mWindowWidth), static_cast<uint16_t>(mWindowHeight));

            mWindowResized = false;
        }

        // Dummy draw call to make sure view 0 is cleared
        bgfx::touch(0);

        drawScene();

        // Advance to next frame
        bgfx::frame();

        mFrameCounter++;
	}

	void BGFXRenderSystem::drawScene()
	{
        // Set Camera Matrices
        bgfx::setViewTransform(0, mEditorCamMats.view, mEditorCamMats.proj);

        // Set Light Uniforms
        setupLightUniformsForDraw();

        // Generate Mesh Batches (Batches of entities which share same mesh and material) for this frame
        std::vector<MeshDrawBatch> batches;
        buildBatches(batches);

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
                bgfx::setTexture(tidx, mTexAlbedoSampler, mTexData[id].handle);

                tidx++;
            }

            // Render using instanced rendering if supported/enableded, and there is more than one entity in batch
            if (mUseInstancing && mSupportsInstancing && drawBatch.entities.size() > 1)
            {
	            drawMeshBatchInstanced(drawBatch);
            }
            // Else use normal rendering path
            else
            {
	            drawMeshBatch(drawBatch);
            }

            // Discard Vertex/Index/Buffers/Textures after rendering all entities in batch
            bgfx::discard(BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_BINDINGS);
        }

        bgfx::discard(BGFX_DISCARD_ALL);
	}

	void BGFXRenderSystem::buildBatches(std::vector<MeshDrawBatch>& batches)
	{
		batches.clear();

        batches.reserve(mMeshData.size() * mMatData.size());

        std::unordered_map<MeshMatPair, std::set<ECS::EntityID>> meshMatMap;

        // Generate set of entities for each unique mesh/material pair
        for (const auto& meshData : mMeshData)
        {
	        const std::set<ECS::EntityID>& meshEntities = mMeshSets[meshData.assetId];

            for (const ECS::EntityID& entity : meshEntities)
            {
                auto& mesh = mWorld->GetComponent<MeshComponent>(entity);

                MeshMatPair pair(mesh.textureAssetId, meshData.assetId);

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
            batch.meshData = mMeshData[meshID];
            batch.matData = mMatData[matID];
            batch.entities = snd;

            batches.push_back(batch);
        }
	}

	void BGFXRenderSystem::drawMeshBatch(const MeshDrawBatch& meshDrawBatch) const
	{
        for (const auto& entity : meshDrawBatch.entities)
        {
            const auto& transform = mWorld->GetComponent<TransformComponent>(entity);

            // Setup Transform
            float model[16];

            buildModelTransform(transform, model);

            bgfx::setTransform(model);

            // Submit Program
            bgfx::submit(0, meshDrawBatch.matData.programHandle, 0, BGFX_DISCARD_TRANSFORM | BGFX_DISCARD_STATE);
        }
	}

	void BGFXRenderSystem::drawMeshBatchInstanced(const MeshDrawBatch& meshDrawBatch)
	{
        // Setup stride for instanced rendering
        constexpr uint16_t instanceStride = 80; // 64 Bytes for 4x4 matrix, 16 Bytes for Instance Index

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
            auto mtx = reinterpret_cast<float*>(data);

            buildModelTransform(transform, mtx);

            auto* index = reinterpret_cast<float*>(&data[64]);
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

	void BGFXRenderSystem::initMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
        auto& mesh = entity->GetComponent<MeshComponent>();

        // Init Mesh
        if (!mMeshData.contains(mesh.meshAssetId))
        {
			MeshDataBGFX meshData;

			loadAndInitMesh(mesh.meshAssetId, meshData);

            mMeshData.insert(mesh.meshAssetId, meshData);
            mMeshSets.emplace(mesh.meshAssetId, std::set<ECS::EntityID>());
        }

        mMeshSets[mesh.meshAssetId].insert(entity->ID());

        // Init Textures
        if (!mTexData.contains(mesh.textureAssetId))
        {
			TextureDataBGFX texData;

            loadAndInitTexture(mesh.textureAssetId, texData);

            mTexData.insert(mesh.textureAssetId, texData);
            mTexSets.emplace(mesh.textureAssetId, std::set<ECS::EntityID>());
        }

        mTexSets[mesh.textureAssetId].insert(entity->ID());

        // Init Materials (Currently using default mesh program, will add proper material system later)
        if (!mMatData.contains(mesh.textureAssetId))
        {
	        MaterialDataBGFX matData;

            matData.assetId = mesh.textureAssetId;

            if (mUseInstancing && mSupportsInstancing)
            {
                matData.programHandle = mMeshInstancedProgram;
            }
            else
            {
                matData.programHandle = mMeshProgram;
            }

            matData.texIDs.push_back(mesh.textureAssetId);

            mMatData.insert(mesh.textureAssetId, matData);
            mMatSets.emplace(mesh.textureAssetId, std::set<ECS::EntityID>());
        }

        mMatSets[mesh.textureAssetId].insert(entity->ID());
	}

	void BGFXRenderSystem::cleanupMeshComponent(std::shared_ptr<ECS::Entity> entity)
	{
		const auto& mesh = entity->GetComponent<MeshComponent>();

        // Cleanup Mesh Data
        mMeshSets[mesh.meshAssetId].erase(entity->ID());

        if (mMeshSets[mesh.meshAssetId].empty())
        {
            bgfx::destroy(mMeshData[mesh.meshAssetId].vertexBufferHandle);
            bgfx::destroy(mMeshData[mesh.meshAssetId].indexBufferHandle);

            mMeshData.erase(mesh.meshAssetId);
            mMeshSets.erase(mesh.meshAssetId);
        }

        // Cleanup Material Data
        mMatSets[mesh.textureAssetId].erase(entity->ID());

        if (mMatSets[mesh.textureAssetId].empty())
        {
			mMatData.erase(mesh.textureAssetId);
            mMatSets.erase(mesh.textureAssetId);
        }

        // Cleanup Texture Data
        mTexSets[mesh.textureAssetId].erase(entity->ID());

        if (mTexSets[mesh.textureAssetId].empty())
        {
			bgfx::destroy(mTexData[mesh.textureAssetId].handle);

            mTexData.erase(mesh.textureAssetId);
            mTexSets.erase(mesh.textureAssetId);
        }
	}

	void BGFXRenderSystem::initEditorCamera()
	{
        mEditorCam.position = {0.0f, 0.0f, 15.0f};
        mEditorCam.aspect = static_cast<float>(mWindowWidth) / static_cast<float>(mWindowHeight);
        mEditorCam.lookAt = mEditorCam.position + mEditorCam.direction;

        bx::mtxLookAt(mEditorCamMats.view, static_cast<bx::Vec3>(mEditorCam.position), static_cast<bx::Vec3>(mEditorCam.lookAt), { 0, 1, 0 }, bx::Handedness::Right);
        bx::mtxProj(mEditorCamMats.proj, mEditorCam.fovY, mEditorCam.aspect, mEditorCam.zNear, mEditorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
	}

	void BGFXRenderSystem::updateEditorCamera()
    {
	    if (const auto inputSubsystem = mEngine->getSubsystem<input::InputSubsystem>(); inputSubsystem->isCursorLocked())
        {
            // Camera Movement
            if (mMoveLeft && !mMoveRight)
            {
                mEditorCam.position += mEditorCam.right * mEditorCam.speed * mEngine->deltaTime();
            }

            if (mMoveRight && !mMoveLeft)
            {
                mEditorCam.position -= mEditorCam.right * mEditorCam.speed * mEngine->deltaTime();
            }

            if (mMoveForward && !mMoveBackward)
            {
                mEditorCam.position += mEditorCam.direction * mEditorCam.speed * mEngine->deltaTime();
            }

            if (mMoveBackward && !mMoveForward)
            {
                mEditorCam.position -= mEditorCam.direction * mEditorCam.speed * mEngine->deltaTime();
            }

            if (mMoveUp && !mMoveDown)
            {
                mEditorCam.position += mEditorCam.up * mEditorCam.speed * mEngine->deltaTime();
            }

            if (mMoveDown && !mMoveUp)
            {
                mEditorCam.position -= mEditorCam.up * mEditorCam.speed * mEngine->deltaTime();
            }

            // Mouse Rotation
            mEditorCam.yaw += inputSubsystem->getMouseXOffset();
            mEditorCam.pitch -= inputSubsystem->getMouseYOffset();

            if (mEditorCam.pitch > 89.0f)
                mEditorCam.pitch = 89.0f;

            if (mEditorCam.pitch < -89.0f)
                mEditorCam.pitch = -89.0f;

            // Calculate Direction vector from yaw and pitch of camera
            mEditorCam.direction.x = cos(maths::DegreesToRadians(mEditorCam.yaw)) * cos(maths::DegreesToRadians(mEditorCam.pitch));
            mEditorCam.direction.y = sin(maths::DegreesToRadians(mEditorCam.pitch));
            mEditorCam.direction.z = sin(maths::DegreesToRadians(mEditorCam.yaw)) * cos(maths::DegreesToRadians(mEditorCam.pitch));

            mEditorCam.direction.Normalise();
        }

        // Calculate Right, Up and LookAt vectors
        mEditorCam.right = mEditorCam.up.Cross(mEditorCam.direction).Normalised();
        mEditorCam.lookAt = mEditorCam.position + mEditorCam.direction;

        if (mWindowResized)
        {
            mEditorCam.aspect = static_cast<float>(mWindowWidth) / static_cast<float>(mWindowHeight);
        }

        bx::mtxLookAt(mEditorCamMats.view, static_cast<bx::Vec3>(mEditorCam.position), static_cast<bx::Vec3>(mEditorCam.lookAt), {0, 1, 0}, bx::Handedness::Right);
        bx::mtxProj(mEditorCamMats.proj, mEditorCam.fovY, mEditorCam.aspect, mEditorCam.zNear, mEditorCam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
    }

	void BGFXRenderSystem::updateCameraComponent(const std::shared_ptr<ECS::Entity>& entity) const
	{
		const auto& transform = entity->GetComponent<TransformComponent>();
        auto& cam = entity->GetComponent<CameraComponent>();
        auto& camMats = entity->GetComponent<CameraMatComponent>();

        // Calculate Right, Up and LookAt vectors
        cam.right = cam.up.Cross(transform.rotation.xyz()).Normalised();
        cam.lookAt = transform.position + transform.rotation.xyz();

        bx::mtxLookAt(camMats.view, static_cast<bx::Vec3>(transform.position), static_cast<bx::Vec3>(cam.lookAt), { 0, 1, 0 }, bx::Handedness::Right);

        // Recalculate camera perspective if fov has changed, store new fov in prevFov
        if (cam.fovY != cam.prevFovY)
        {
            cam.prevFovY = cam.fovY;

            bx::mtxProj(camMats.proj, cam.fovY, cam.aspect, cam.zNear, cam.zFar, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Right);
        }
	}

	void BGFXRenderSystem::loadAndInitMesh(const UUID meshId, MeshDataBGFX& meshData)
	{
        const auto meshAsset = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshId));

        if (meshAsset && meshAsset->load())
        {
            meshData.assetId = meshId;
            meshData.numVertices = meshAsset->numVertices();
            meshData.numIndices = meshAsset->numIndices();

            meshData.vertexBufferHandle = initVertexBuffer(meshAsset->vertices().data(), meshData.numVertices, gLayoutVertexPNTV32);
            meshData.indexBufferHandle = initIndexBuffer(meshAsset->indices().data(), meshData.numIndices, true);

            meshAsset->unload();
        }
	}

	bgfx::VertexBufferHandle BGFXRenderSystem::initVertexBuffer(const void* vertices, const uint32_t& numVertices,
		const bgfx::VertexLayout& layout)
	{
		// Allocate memory for vertex buffer
        const bgfx::Memory* mem = bgfx::alloc(numVertices * layout.getStride());

        // Copy vertices to allocated memory
        bx::memCopy(mem->data, vertices, mem->size);

        // Create vertex buffer
        return bgfx::createVertexBuffer(mem, layout);
	}

	bgfx::IndexBufferHandle BGFXRenderSystem::initIndexBuffer(const void* indices, const uint32_t numIndices,
	                                                          const bool use32BitIndices)
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

    void BGFXRenderSystem::loadAndInitTexture(UUID texId, TextureDataBGFX& texData)
    {
        const auto texAsset = std::static_pointer_cast<assets::TextureAsset>(assets::AssetRegistry::get()->getAsset(texId));

        if (texAsset && texAsset->load())
        {
            texData.assetId = texId;

            const uint32_t texSize = texAsset->textureSize();

            const bgfx::Memory* mem = bgfx::alloc(texSize);

            bx::memCopy(mem->data, texAsset->pixelData(), mem->size);

            texData.handle = bgfx::createTexture2D(texAsset->textureWidth(), texAsset->textureHeight(),
                false, 1, gTexFormatBGFX.at(texAsset->textureFormat()), 0, mem);

            texAsset->unload();
        }
    }

    void BGFXRenderSystem::setupLightUniformsForDraw() const
    {
		// Setup Light Data
        float lightPos[gMaxLightsBGFX][4];
        float lightDir[gMaxLightsBGFX][4];
        float lightColor[gMaxLightsBGFX][4];
        float lightAmbientSpec[gMaxLightsBGFX][4];
        float lightAttenuation[gMaxLightsBGFX][4];
        float lightIndex[4];

        int index = 0;

        PackedVector<ECS::EntityPtr> lightEntities;
        ECS::GetEntities<TransformComponent, LightComponent>(mWorld, lightEntities);

        std::vector<ECS::EntityPtr> lightEntitiesOrdered;

        // Sort Lights into DIRECTIONAL, POINT, SPOT Order
        lightEntitiesOrdered.reserve(lightEntities.size());
        
        for (const auto& entity : lightEntities)
        {
            // Break out of loop when number of lights exceeds max
            if (index >= gMaxLightsBGFX)
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
            if (index >= gMaxLightsBGFX)
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
            if (index >= gMaxLightsBGFX)
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
			if (index >= gMaxLightsBGFX)
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
        
		bgfx::setUniform(mLightUniformHandles.position, lightPos, gMaxLightsBGFX);
        bgfx::setUniform(mLightUniformHandles.direction, lightDir, gMaxLightsBGFX);
        bgfx::setUniform(mLightUniformHandles.color, lightColor, gMaxLightsBGFX);
        bgfx::setUniform(mLightUniformHandles.ambientSpecular, lightAmbientSpec, gMaxLightsBGFX);
        bgfx::setUniform(mLightUniformHandles.attenuation, lightAttenuation, gMaxLightsBGFX);
        bgfx::setUniform(mLightUniformHandles.index, lightIndex, 1);
    }

    void BGFXRenderSystem::buildModelTransform(const TransformComponent& transform, float* model)
	{
        // Identity
		bx::mtxIdentity(model);

        // Scale
        bx::mtxScale(model, transform.scale.x, transform.scale.y, transform.scale.z);

        Vector3f euler = transform.rotation.eulerAnglesRad();

        // Rotation
        bx::mtxRotateZ(model, euler.z);
        bx::mtxRotateX(model, euler.x);
        bx::mtxRotateY(model, euler.y);

        // Translation
        bx::mtxTranslate(model, static_cast<float>(transform.position.x), static_cast<float>(transform.position.y), static_cast<float>(transform.position.z));
	}
}
