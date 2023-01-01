cmake_minimum_required(VERSION 3.24.2)

# Download CPM.cmake
set(CPM_DOWNLOAD_VERSION 0.36.0)

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION}
  )
endif()

include(${CPM_DOWNLOAD_LOCATION})

# Define Add Submodule Function
function(add_git_submodule dir)

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)

if(EXISTS ${dir}/CMakeLists.txt)
    add_subdirectory(${dir})
endif()

endfunction(add_git_submodule)

function(checkout_git_branch dir branch)

find_package(Git REQUIRED)

execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${branch}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
    COMMAND_ERROR_IS_FATAL ANY)

endfunction(checkout_git_branch)

# Init/Update Submodules
add_git_submodule("deps/box2d")
add_git_submodule("deps/implot")
add_git_submodule("deps/OpenSimplexNoise")

add_git_submodule("deps/imgui")
checkout_git_branch("deps/imgui" "docking")

add_git_submodule("deps/imgui-filebrowser")
add_git_submodule("deps/lz4")
add_git_submodule("deps/assimp")
add_git_submodule("deps/bgfx.cmake")

# Define CMAKE Variables
set(PROJECT_NAME PuffinEngine)
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.3.216.0")

# Set Project Name
project (${PROJECT_NAME} DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)

# Set C++ Language Standard to C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
set(IMGUI_VULKAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(VKBOOTSTRAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vk-bootstrap)
set(ANGELSCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/angelscript)
set(OPENSIMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenSimplexNoise/OpenSimplexNoise)

set(HEADERS 
	# Assets
	${HEADER_DIR}/Assets/Asset.h
	${HEADER_DIR}/Assets/AssetRegistry.h
	${HEADER_DIR}/Assets/MeshAsset.h
	${HEADER_DIR}/Assets/SoundAsset.h
	${HEADER_DIR}/Assets/TextureAsset.h
	${HEADER_DIR}/Assets/Importers/ModelImporter.h
	${HEADER_DIR}/Assets/Importers/TextureImporter.h
	# Audio
	${HEADER_DIR}/Audio/AudioSubsystem.h
	# Components
	${HEADER_DIR}/Components/TransformComponent.h
	${HEADER_DIR}/Components/Rendering/CameraComponent.h
	${HEADER_DIR}/Components/Rendering/LightComponent.h
	${HEADER_DIR}/Components/Rendering/MeshComponent.h
	${HEADER_DIR}/Components/Physics/RigidbodyComponent2D.h
	${HEADER_DIR}/Components/Physics/ShapeComponents2D.h
	${HEADER_DIR}/Components/Physics/VelocityComponent.hpp
	${HEADER_DIR}/Components/Procedural/ProceduralMeshComponent.hpp
	${HEADER_DIR}/Components/Scripting/AngelScriptComponent.hpp
	${HEADER_DIR}/Components/Scripting/NativeScriptComponent.hpp
	# ECS
	${HEADER_DIR}/ECS/ComponentType.h
	${HEADER_DIR}/ECS/ECS.h
	${HEADER_DIR}/ECS/Entity.h
	${HEADER_DIR}/ECS/EntityID.h
	${HEADER_DIR}/ECS/System.hpp
	# Engine
	${HEADER_DIR}/Engine/Engine.hpp
	${HEADER_DIR}/Engine/Application.hpp
	${HEADER_DIR}/Engine/Subsystem.hpp
	${HEADER_DIR}/Engine/EventSubsystem.hpp
	# Input
	${HEADER_DIR}/Input/InputEvent.h
	${HEADER_DIR}/Input/InputSubsystem.h
	# Physics
	${HEADER_DIR}/Physics/CollisionEvent.h
	${HEADER_DIR}/Physics/Onager2D/PhysicsTypes2D.h
	${HEADER_DIR}/Physics/Onager2D/Onager2DPhysicsSystem.h
	${HEADER_DIR}/Physics/Onager2D/PhysicsHelpers2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/Shape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/PolygonShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/CircleShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Shapes/BoxShape2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/Collider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/PolygonCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/BoxCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Colliders/CircleCollider2D.h
	${HEADER_DIR}/Physics/Onager2D/Broadphases/Broadphase2D.hpp
	${HEADER_DIR}/Physics/Onager2D/Broadphases/PruneAndSweepBroadphase.h
	${HEADER_DIR}/Physics/Box2D/Box2DPhysicsSystem.h
	${HEADER_DIR}/Physics/Box2D/Box2DContactListener.h
	# Procedural
	${HEADER_DIR}/Procedural/ProceduralMeshGenSystem.hpp
	# Rendering
	${HEADER_DIR}/Rendering/BGFX/BGFXRenderSystem.hpp
	${HEADER_DIR}/Rendering/Vulkan/VulkanRenderSystem.h
	${HEADER_DIR}/Rendering/Vulkan/VKTypes.h
	${HEADER_DIR}/Rendering/Vulkan/VKBaseRenderer.h
	${HEADER_DIR}/Rendering/Vulkan/VKDeferredRender.h
	${HEADER_DIR}/Rendering/Vulkan/VKForwardRenderer.h
	${HEADER_DIR}/Rendering/Vulkan/DebugDraw.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKDebug.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKDescriptors.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKHelper.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKInitializers.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKPipeline.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKTexture.h
	${HEADER_DIR}/Rendering/Vulkan/Helpers/VKVertex.hpp
	# Scripting
	${HEADER_DIR}/Scripting/RegisterTypeHelpers.h
	${HEADER_DIR}/Scripting/AngelScriptSystem.h
	${HEADER_DIR}/Scripting/NativeScriptSystem.hpp
	# Types
	${HEADER_DIR}/Types/ComponentFlags.h
	${HEADER_DIR}/Types/Matrix.h
	${HEADER_DIR}/Types/PackedArray.h
	${HEADER_DIR}/Types/Quat.h
	${HEADER_DIR}/Types/RingBuffer.h
	${HEADER_DIR}/Types/Simplex.h
	${HEADER_DIR}/Types/UUID.h
	${HEADER_DIR}/Types/Vector.h
	${HEADER_DIR}/Types/Vertex.hpp
	# UI
	${HEADER_DIR}/UI/Editor/UIManager.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindow.h
	${HEADER_DIR}/UI/Editor/Windows/UIContentBrowser.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowEntityProperties.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowPerformance.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowSceneHierarchy.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowSettings.h
	${HEADER_DIR}/UI/Editor/Windows/UIWindowViewport.h
	# Window
	${HEADER_DIR}/Window/WindowSubsystem.hpp
	# Root
	${HEADER_DIR}/ManipulationGizmo.h
	${HEADER_DIR}/MathHelpers.h
	${HEADER_DIR}/ProjectSettings.h
	${HEADER_DIR}/SerializeScene.h
	)

set(SOURCES 
	# Assets
	${SOURCE_DIR}/Assets/AssetRegistry.cpp
	${SOURCE_DIR}/Assets/MeshAsset.cpp
	${SOURCE_DIR}/Assets/TextureAsset.cpp
	# Audio
	${SOURCE_DIR}/Audio/AudioSubsystem.cpp
	# Engine
	${SOURCE_DIR}/Engine/Engine.cpp
	${SOURCE_DIR}/Engine/Application.cpp
	# Input
	${SOURCE_DIR}/Input/InputSubsystem.cpp
	# Physics
	${SOURCE_DIR}/Physics/Onager2D/Onager2DPhysicsSystem.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/CircleShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/BoxShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Shapes/CircleShape2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/BoxCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/CircleCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Colliders/PolygonCollider2D.cpp
	${SOURCE_DIR}/Physics/Onager2D/Broadphases/PruneAndSweepBroadphase.cpp
	${SOURCE_DIR}/Physics/Box2D/Box2DContactListener.cpp
	${SOURCE_DIR}/Physics/Box2D/Box2DPhysicsSystem.cpp
	# Procedural
	${SOURCE_DIR}/Procedural/ProceduralMeshGenSystem.cpp
	# Rendering
	${SOURCE_DIR}/Rendering/BGFX/BGFXRenderSystem.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VulkanRenderSystem.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VkDeferredRender.cpp
	${SOURCE_DIR}/Rendering/Vulkan/VKForwardRenderer.cpp
	${SOURCE_DIR}/Rendering/Vulkan/Helpers/VKDescriptors.cpp
	${SOURCE_DIR}/Rendering/Vulkan/Helpers/VKHelper.cpp
	${SOURCE_DIR}/Rendering/Vulkan/Helpers/VKVertex.cpp
	# Scripting
	${SOURCE_DIR}/Scripting/AngelScriptSystem.cpp
	${SOURCE_DIR}/Scripting/NativeScriptSystem.cpp
	# UI
	${SOURCE_DIR}/UI/Editor/UIManager.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindow.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIContentBrowser.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowEntityProperties.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowPerformance.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowSceneHierarchy.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowSettings.cpp
	${SOURCE_DIR}/UI/Editor/Windows/UIWindowViewport.cpp
	# Window
	${SOURCE_DIR}/Window/WindowSubsystem.cpp
	# Root
	${SOURCE_DIR}/main.cpp 
	${SOURCE_DIR}/ManipulationGizmo.cpp
	)
	
set (IMGUI_SOURCES
	${IMGUI_DIR}/imgui.h
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.h
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
	)
	
set (IMGUI_VULKAN_SOURCES
	${IMGUI_VULKAN_DIR}/imgui_impl_vulkan.cpp
	)
	
set (OPENSIMPLEX_SOURCES
	${OPENSIMPLEX_DIR}/OpenSimplexNoise.cpp
	)
	
set (VKBOOTSTRAP_SOURCES
	${VKBOOTSTRAP_DIR}/VkBootstrap.cpp
	)
	
set (ANGELSCRIPT_SOURCES
	${ANGELSCRIPT_DIR}/scriptarray/scriptarray.cpp
	${ANGELSCRIPT_DIR}/scriptbuilder/scriptbuilder.cpp
	${ANGELSCRIPT_DIR}/scriptstdstring/scriptstdstring.cpp
	${ANGELSCRIPT_DIR}/scriptstdstring/scriptstdstring_utils.cpp
	)

# Add Main as Executable
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
#target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_VULKAN_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PROJECT_NAME} PRIVATE ${ANGELSCRIPT_SOURCES})

message("")
message("============================================================")
message("Sorting Source Files into relative Subfolders")
message("============================================================")

function(sort_into_source_group files dir sort_dir)

foreach(file IN LISTS ${files})
	message("")
	message("Generating Source Group for " ${file})
	file(RELATIVE_PATH relative_path ${dir} ${file})
	get_filename_component(relative_dir ${relative_path} DIRECTORY)
	message("Relative Path: " ${relative_path})
	message("Relative Dir: " ${relative_dir})
    string(REPLACE "/" "\\" relative_path_msvc "${relative_dir}")
    source_group("${sort_dir}/${relative_path_msvc}" FILES "${file}")
endforeach()

endfunction(sort_into_source_group)

sort_into_source_group(HEADERS ${HEADER_DIR} "")
sort_into_source_group(SOURCES ${SOURCE_DIR} "")
sort_into_source_group(IMGUI_SOURCES ${IMGUI_DIR} Libraries/ImGui)
#sort_into_source_group(IMGUI_VULKAN_SOURCES ${IMGUI_VULKAN_DIR} Libraries/ImGui)
sort_into_source_group(OPENSIMPLEX_SOURCES ${OPENSIMPLEX_DIR} Libraries/OpenSimplexNoise)
sort_into_source_group(VKBOOTSTRAP_SOURCES ${VKBOOTSTRAP_DIR} Libraries/VKBootstrap)
sort_into_source_group(ANGELSCRIPT_SOURCES ${ANGELSCRIPT_DIR} Libraries/AngelScript)

message("")

# Get & Build Required Libraries
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:nlohmann/json@3.11.2")

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME} ${LIB_DIR}/irrklang/irrKlang.lib)

# Define Debug Libs
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/box2d/Debug/box2d.lib)
#target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/assimp/Debug/assimp-vc143-mtd.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/glfw/Debug/glfw3.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/angelscript/Debug/angelscript64d.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/lz4/Debug/liblz4_static.lib)

target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bgfx.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bimg.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/bx.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/astc-encoder.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/edtaa3.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/etc1.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/etc2.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/iqa.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/nvtt.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/pvrtc.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/squish.lib)
target_link_libraries(${PROJECT_NAME} debug ${LIB_DIR}/bgfx/Debug/tinyexr.lib)

# Define Release Libs
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/box2d/Release/box2d.lib)
#target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/assimp/Release/assimp-vc143-mt.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/glfw/Release/glfw3.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/angelscript/Release/angelscript64.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/lz4/Release/liblz4_static.lib)

target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bgfx.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bimg.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/bx.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/astc-encoder.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/edtaa3.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/etc1.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/etc2.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/iqa.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/nvtt.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/pvrtc.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/squish.lib)
target_link_libraries(${PROJECT_NAME} optimized ${LIB_DIR}/bgfx/Release/tinyexr.lib)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Add include paths
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE third_party)
target_include_directories(${PROJECT_NAME} PRIVATE third_party/angelscript)
target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_SDK_PATH}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/box2d/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/OpenSimplexNoise)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/imgui-filebrowser)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/lz4/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bgfx/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bx/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/bgfx.cmake/bimg/include)
#target_include_directories(${PROJECT_NAME} PRIVATE ${DEPS_DIR}/assimp/include)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_compile_definitions(${PROJECT_NAME} PRIVATE PFN_USE_DOUBLE_PRECISION)
option(PFN_USE_DOUBLE_PRECISION "Enabled Double Precision Support" ON)

set_property(TARGET ${PROJECT_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE /MP)

if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
else ()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wa,-mbig-obj)
endif ()