cmake_minimum_required(VERSION 3.24)

include(FetchContent)

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(${CMAKE_CURRENT_SOURCE_DIR}/PuffinEngine.cmake)

# Init/Update Submodules
add_git_submodule("deps/box2d")
add_git_submodule("deps/enkits")
add_git_submodule("deps/entt")
add_git_submodule("deps/imgui")
add_git_submodule("deps/imgui-filebrowser")
add_git_submodule("deps/implot")
add_git_submodule("deps/json")
add_git_submodule("deps/lz4")
add_git_submodule("deps/OpenSimplexNoise")
add_git_submodule("deps/tinygltf")
add_git_submodule("deps/vkb")
add_git_submodule("deps/vma-hpp")
add_git_submodule("deps/jolt")

#checkout_git_branch("deps/imgui" "docking")

# Define CMAKE Variables
set(PUFFIN_ENGINE_NAME PuffinEngine)
set(PUFFIN_EDITOR_NAME PuffinEditor)

# Set Executable/Library/Archive Output Directories
if (WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/release)
elseif (LINUX)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/linux/release)
endif()

# Options
option(DOUBLE_PRECISION_SUPPORT "Enable Double Precision Support" OFF)
option(BOX2D_PHYSICS_SUPPORT "Compile with Box2D Physics Support" ON)
option(JOLT_PHYSICS_SUPPORT "Compile with Jolt Physics Support" OFF)
option(ONAGER2D_PHYSICS_SUPPORT "Compile with Onager 2D Physics Support" OFF)

set(PFN_PLATFORM "Raylib" CACHE STRING "Platform to build puffin for.")
set_property(CACHE PFN_PLATFORM PROPERTY STRINGS Raylib)

set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
#set(VKBOOTSTRAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/vkb/src)
set(OPENSIMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenSimplexNoise/OpenSimplexNoise)
set(ENKITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/enkits)

set(ENGINE_HEADERS
	# Root
	${HEADER_DIR}/puffin/manipulationgizmo.h
	${HEADER_DIR}/puffin/mathhelpers.h
	${HEADER_DIR}/puffin/projectsettings.h
	# Assets
	${HEADER_DIR}/puffin/assets/asset.h
	${HEADER_DIR}/puffin/assets/assetimporters.h
	${HEADER_DIR}/puffin/assets/assetregistry.h
	${HEADER_DIR}/puffin/assets/materialasset.h
	${HEADER_DIR}/puffin/assets/staticmeshasset.h
	${HEADER_DIR}/puffin/assets/shaderasset.h
	${HEADER_DIR}/puffin/assets/soundasset.h
	${HEADER_DIR}/puffin/assets/textureasset.h
	# Audio
	${HEADER_DIR}/puffin/audio/audiosubsystem.h
	${HEADER_DIR}/puffin/audio/miniaudiosubsystem.h
	# Components
	${HEADER_DIR}/puffin/components/transformcomponent2d.h
	${HEADER_DIR}/puffin/components/transformcomponent3d.h
	# Components - Physics - 2D
	${HEADER_DIR}/puffin/components/physics/2d/boxcomponent2d.h
	${HEADER_DIR}/puffin/components/physics/2d/circlecomponent2d.h
	${HEADER_DIR}/puffin/components/physics/2d/rigidbodycomponent2d.h
	${HEADER_DIR}/puffin/components/physics/2d/shapecomponent2d.h
	${HEADER_DIR}/puffin/components/physics/2d/velocitycomponent2d.h
	# Components - Physics - 3D
	${HEADER_DIR}/puffin/components/physics/3d/boxcomponent3d.h
	${HEADER_DIR}/puffin/components/physics/3d/rigidbodycomponent3d.h
	${HEADER_DIR}/puffin/components/physics/3d/shapecomponent3d.h
	${HEADER_DIR}/puffin/components/physics/3d/spherecomponent3d.h
	${HEADER_DIR}/puffin/components/physics/3d/velocitycomponent3d.h
	# Components - Procedural
	# Components - Procedural - 3D
	${HEADER_DIR}/puffin/components/procedural/3d/proceduralcubecomponent3d.h
	${HEADER_DIR}/puffin/components/procedural/3d/proceduralicospherecomponent3d.h
	${HEADER_DIR}/puffin/components/procedural/3d/proceduralplanecomponent3d.h
	${HEADER_DIR}/puffin/components/procedural/3d/proceduralterraincomponent3d.h
	${HEADER_DIR}/puffin/components/procedural/3d/proceduraluvspherecomponent3d.h
	# Components - Rendering
	# Components - Rendering - 2D
	${HEADER_DIR}/puffin/components/rendering/2d/cameracomponent2d.h
	${HEADER_DIR}/puffin/components/rendering/2d/rectanglespritecomponent2d.h
	${HEADER_DIR}/puffin/components/rendering/2d/spritecomponent2d.h
	${HEADER_DIR}/puffin/components/rendering/2d/texturedspritecomponent2d.h
	# Components - Rendering - 3D
	${HEADER_DIR}/puffin/components/rendering/3d/cameracomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/directionallightcomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/lightcomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/pointlightcomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/proceduralmeshcomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/shadowcastercomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/spotlightcomponent3d.h
	${HEADER_DIR}/puffin/components/rendering/3d/staticmeshcomponent3d.h
	# Core
	${HEADER_DIR}/puffin/core/application.h
	${HEADER_DIR}/puffin/core/engine.h
	${HEADER_DIR}/puffin/core/enginehelpers.h
	${HEADER_DIR}/puffin/core/enkitssubsystem.h
	${HEADER_DIR}/puffin/core/settingsmanager.h
	${HEADER_DIR}/puffin/core/signalsubsystem.h
	${HEADER_DIR}/puffin/core/subsystem.h
	${HEADER_DIR}/puffin/core/subsystemmanager.h
	${HEADER_DIR}/puffin/core/timer.h
	# ECS
	${HEADER_DIR}/puffin/ecs/enttsubsystem.h
	# Gameplay
	# Input
	${HEADER_DIR}/puffin/input/inputcontext.h
	${HEADER_DIR}/puffin/input/inputevent.h
	${HEADER_DIR}/puffin/input/inputsubsystem.h
	${HEADER_DIR}/puffin/input/inputtypes.h
	# Nodes
	${HEADER_DIR}/puffin/nodes/node.h
	${HEADER_DIR}/puffin/nodes/transformnode2d.h
	${HEADER_DIR}/puffin/nodes/transformnode3d.h
	# Nodes - Physics
	${HEADER_DIR}/puffin/nodes/physics/rigidbodynode3d.h
	# Nodes - Rendering
	# Nodes - Rendering - 2D
	${HEADER_DIR}/puffin/nodes/rendering/2d/spritenode2d.h
	# Nodes - Rendering - 3D
	${HEADER_DIR}/puffin/nodes/rendering/3d/cameranode3d.h
	${HEADER_DIR}/puffin/nodes/rendering/3d/directionallightnode3d.h
	${HEADER_DIR}/puffin/nodes/rendering/3d/lightnode3d.h
	${HEADER_DIR}/puffin/nodes/rendering/3d/pointlightnode3d.h
	${HEADER_DIR}/puffin/nodes/rendering/3d/spotlightnode3d.h
	${HEADER_DIR}/puffin/nodes/rendering/3d/staticmeshnode3d.h
	# Physics
	${HEADER_DIR}/puffin/physics/bodytype.h
	${HEADER_DIR}/puffin/physics/collisionevent.h
	${HEADER_DIR}/puffin/physics/physicsconstants.h
	${HEADER_DIR}/puffin/physics/shapetype2d.h
	# Physics - Box2D
	${HEADER_DIR}/puffin/physics/box2d/box2dphysicssubsystem.h
	# Physics - Jolt
	${HEADER_DIR}/puffin/physics/jolt/joltphysicssubsystem.h
	${HEADER_DIR}/puffin/physics/jolt/joltphysicstypes.h
	# Physics - Onager2D
	${HEADER_DIR}/puffin/physics/onager2d/onager2dphysicssubsystem.h
	${HEADER_DIR}/puffin/physics/onager2d/physicshelpers2d.h
	${HEADER_DIR}/puffin/physics/onager2d/physicstypes2d.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/broadphase2d.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/sweepandprunebroadphase2d.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/spatialhashbroadphase2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/collider2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/polygoncollider2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/boxcollider2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/circlecollider2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/shape2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/polygonshape2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/circleshape2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/boxshape2d.h
	# Platform
	${HEADER_DIR}/puffin/platform/platform.h
	# Procedural
	${HEADER_DIR}/puffin/procedural/proceduralmeshgensubsystem.h
	# Rendering
	${HEADER_DIR}/puffin/rendering/camerasubsystem.h
	${HEADER_DIR}/puffin/rendering/materialglobals.h
	${HEADER_DIR}/puffin/rendering/renderglobals.h
	${HEADER_DIR}/puffin/rendering/renderhelpers.h
	${HEADER_DIR}/puffin/rendering/renderpasstype.h
	${HEADER_DIR}/puffin/rendering/rendersubsystem.h
	${HEADER_DIR}/puffin/rendering/resourceid.h
	# Rendering - Vulkan
	#${HEADER_DIR}/puffin/rendering/vulkan/descriptorsvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/helpersvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/logvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/materialregistryvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/pipelinevk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/rendersubsystemvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/resourcedescvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/resourcemanagervk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/texturemanagervk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/typesvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/unifiedgeometrybuffervk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/vertexvk.h
	# Rendering - Vulkan - Render Graph
	#${HEADER_DIR}/puffin/rendering/vulkan/rendergraph/rendergraphvk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/rendergraph/renderpassvk.h
	# Rendering - Vulkan - Render Module
	#${HEADER_DIR}/puffin/rendering/vulkan/rendermodule/corerendermodulevk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.h
	#${HEADER_DIR}/puffin/rendering/vulkan/rendermodule/rendermodulevk.h
	# Scene
	${HEADER_DIR}/puffin/scene/scenegraphgameplaysubsystem.h
	${HEADER_DIR}/puffin/scene/scenegraphsubsystem.h
	${HEADER_DIR}/puffin/scene/sceneinfo.h
	${HEADER_DIR}/puffin/scene/sceneserializationsubsystem.h
	# Serialization
	${HEADER_DIR}/puffin/serialization/componentserialization.h
	# Types
	${HEADER_DIR}/puffin/types/aabb.h
	${HEADER_DIR}/puffin/types/deletionqueue.h
	${HEADER_DIR}/puffin/types/eulerangles.h
	${HEADER_DIR}/puffin/types/matrix.h
	${HEADER_DIR}/puffin/types/quat.h
	${HEADER_DIR}/puffin/types/scenetype.h
	${HEADER_DIR}/puffin/types/size.h
	${HEADER_DIR}/puffin/types/uuid.h
	${HEADER_DIR}/puffin/types/vector2.h
	${HEADER_DIR}/puffin/types/vector3.h
	${HEADER_DIR}/puffin/types/vertex.h
	# Types - Storage
	${HEADER_DIR}/puffin/types/storage/mappedarray.h
	${HEADER_DIR}/puffin/types/storage/mappedvector.h
	${HEADER_DIR}/puffin/types/storage/ringbuffer.h
	# Utility
	${HEADER_DIR}/puffin/utility/benchmark.h
	${HEADER_DIR}/puffin/utility/reflection.h
	${HEADER_DIR}/puffin/utility/serialization.h
	# Window
	${HEADER_DIR}/puffin/window/window.h
	${HEADER_DIR}/puffin/window/windowsubsystem.h
	)

set(ENGINE_SOURCES 
	# Root 
	${SOURCE_DIR}/puffin/manipulationgizmo.cpp
	# Assets
	${SOURCE_DIR}/puffin/assets/asset.cpp
	${SOURCE_DIR}/puffin/assets/assetimporters.cpp
	${SOURCE_DIR}/puffin/assets/assetregistry.cpp
	${SOURCE_DIR}/puffin/assets/materialasset.cpp
	${SOURCE_DIR}/puffin/assets/staticmeshasset.cpp
	${SOURCE_DIR}/puffin/assets/shaderasset.cpp
	${SOURCE_DIR}/puffin/assets/textureasset.cpp
	# Audio
	${SOURCE_DIR}/puffin/audio/audiosubsystem.cpp
	${SOURCE_DIR}/puffin/audio/miniaudiosubsystem.cpp
	# Core
	${SOURCE_DIR}/puffin/core/application.cpp
	${SOURCE_DIR}/puffin/core/engine.cpp
	${SOURCE_DIR}/puffin/core/enginehelpers.cpp
	${SOURCE_DIR}/puffin/core/enkitssubsystem.cpp
	${SOURCE_DIR}/puffin/core/settingsmanager.cpp
	${SOURCE_DIR}/puffin/core/subsystem.cpp
	${SOURCE_DIR}/puffin/core/subsystemmanager.cpp
	${SOURCE_DIR}/puffin/core/timer.cpp
	# ECS
	${SOURCE_DIR}/puffin/ecs/enttsubsystem.cpp
	# Gameplay
	# Input
	${SOURCE_DIR}/puffin/input/inputcontext.cpp
	${SOURCE_DIR}/puffin/input/inputsubsystem.cpp
	# Nodes
	${SOURCE_DIR}/puffin/nodes/node.cpp
	${SOURCE_DIR}/puffin/nodes/transformnode2d.cpp
	${SOURCE_DIR}/puffin/nodes/transformnode3d.cpp
	# Nodes - Physics
	${SOURCE_DIR}/puffin/nodes/physics/rigidbodynode3d.cpp
	# Nodes - Rendering
	# Nodes - Rendering - 2D
	${SOURCE_DIR}/puffin/nodes/rendering/2d/spritenode2d.cpp
	# Nodes - Rendering - 3D
	${SOURCE_DIR}/puffin/nodes/rendering/3d/cameranode3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/3d/directionallightnode3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/3d/lightnode3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/3d/pointlightnode3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/3d/spotlightnode3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/3d/staticmeshnode3d.cpp
	# Physics
	# Physics - Box2D
	${SOURCE_DIR}/puffin/physics/box2d/box2dphysicssubsystem.cpp
	# Physics - Jolt
	${SOURCE_DIR}/puffin/physics/jolt/joltphysicssubsystem.cpp
	# Physics - Onager2D
	${SOURCE_DIR}/puffin/physics/onager2d/onager2dphysicssubsystem.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/shapes/circleshape2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/shapes/boxshape2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/boxcollider2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/circlecollider2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/polygoncollider2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/broadphases/sweepandprunebroadphase2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/broadphases/spatialhashbroadphase2d.cpp
	# Platform
	${SOURCE_DIR}/puffin/platform/platform.cpp
	# Procedural
	${SOURCE_DIR}/puffin/procedural/proceduralmeshgensubsystem.cpp
	# Rendering
	${SOURCE_DIR}/puffin/rendering/camerasubsystem.cpp
	${SOURCE_DIR}/puffin/rendering/renderhelpers.cpp
	${SOURCE_DIR}/puffin/rendering/rendersubsystem.cpp
	# Rendering - Vulkan
	#${SOURCE_DIR}/puffin/rendering/vulkan/descriptorsvk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/helpersvk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/materialregistryvk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/pipelinevk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendersubsystemvk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/resourcemanagervk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/texturemanagervk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/unifiedgeometrybuffervk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/vertexvk.cpp
	# Rendering - Vulkan - Render Graph
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendergraph/rendergraphvk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendergraph/renderpassvk.cpp
	# Rendering - Vulkan - Render Module
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendermodule/corerendermodulevk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendermodule/forward3drendermodulevk.cpp
	#${SOURCE_DIR}/puffin/rendering/vulkan/rendermodule/rendermodulevk.cpp
	# Scene
	${SOURCE_DIR}/puffin/scene/scenegraphgameplaysubsystem.cpp
	${SOURCE_DIR}/puffin/scene/scenegraphsubsystem.cpp
	${SOURCE_DIR}/puffin/scene/sceneserializationsubsystem.cpp
	# Serialization
	${SOURCE_DIR}/puffin/serialization/componentserialization.cpp
	# Types
	${SOURCE_DIR}/puffin/types/deletionqueue.cpp
	# Types - Storage
	# UI
	# Utility
	${SOURCE_DIR}/puffin/utility/benchmark.cpp
	${SOURCE_DIR}/puffin/utility/serialization.cpp
	# Window
	${SOURCE_DIR}/puffin/window/window.cpp
	${SOURCE_DIR}/puffin/window/windowsubsystem.cpp
	)

set(EDITOR_HEADERS
	# Editor
	${HEADER_DIR}/puffin/editor/editor.h
	${HEADER_DIR}/puffin/editor/editorcamerasubsystem.h
	# Editor - UI
	${HEADER_DIR}/puffin/editor/ui/editoruisubsystem.h
	${HEADER_DIR}/puffin/editor/ui/nodeeditoruihelpers.h
	# Editor - UI - Windows
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindow.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindowcontentbrowser.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindownodeeditor.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindowperformance.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindowscenehierarchy.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindowsettings.h
	${HEADER_DIR}/puffin/editor/ui/windows/uiwindowviewport.h
	)

set(EDITOR_SOURCES
	# Editor
	${SOURCE_DIR}/puffin/editor/editor.cpp
	${SOURCE_DIR}/puffin/editor/editorcamerasubsystem.cpp
	# Editor - UI
	${SOURCE_DIR}/puffin/editor/ui/editoruisubsystem.cpp
	${SOURCE_DIR}/puffin/editor/ui/nodeeditoruihelpers.cpp
	# Editor - UI - Windows
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindow.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindowcontentbrowser.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindownodeeditor.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindowperformance.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindowscenehierarchy.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindowsettings.cpp
	${SOURCE_DIR}/puffin/editor/ui/windows/uiwindowviewport.cpp
	)
	
set (IMGUI_SOURCES
	${IMGUI_DIR}/imgui.h
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	#${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	#${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.h
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
	)
	
set (OPENSIMPLEX_SOURCES
	${OPENSIMPLEX_DIR}/OpenSimplexNoise.cpp
	)
	
set (VKBOOTSTRAP_SOURCES
	${VKBOOTSTRAP_DIR}/VkBootstrap.cpp
	)
	
set (ENKITS_SOURCES
	${ENKITS_DIR}/src/TaskScheduler.h
	${ENKITS_DIR}/src/TaskScheduler.cpp
	)

if (PFN_PLATFORM STREQUAL "Raylib")

	message("Puffin Platform: Raylib")

	set(CUSTOMIZE_BUILD ON)
	set(SUPPORT_CUSTOM_FRAME_CONTROL 1)

	set (PLATFORM_HEADERS 
		${HEADER_DIR}/puffin/platform/raylib/platformrl.h
		${HEADER_DIR}/puffin/platform/raylib/input/inputsubsystemrl.h
		${HEADER_DIR}/puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.h
		${HEADER_DIR}/puffin/platform/raylib/window/windowrl.h
		${HEADER_DIR}/puffin/platform/raylib/window/windowsubsystemrl.h
	)
	set (PLATFORM_SOURCES 
		${SOURCE_DIR}/puffin/platform/raylib/platformrl.cpp
		${SOURCE_DIR}/puffin/platform/raylib/input/inputsubsystemrl.cpp
		${SOURCE_DIR}/puffin/platform/raylib/rendering/2d/rendersubsystemrl2d.cpp
		${SOURCE_DIR}/puffin/platform/raylib/window/windowrl.cpp
		${SOURCE_DIR}/puffin/platform/raylib/window/windowsubsystemrl.cpp
	)

else()

	message("Puffin Platform: None")
	set (PLATFORM_HEADERS)
	set (PLATFORM_SOURCES)

endif()

project ("Puffin" DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)
add_library(${PUFFIN_ENGINE_NAME} ${ENGINE_HEADERS} ${ENGINE_SOURCES} ${PLATFORM_HEADERS} ${PLATFORM_SOURCES})
add_executable(${PUFFIN_EDITOR_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${SOURCE_DIR}/main.cpp )

# Set C++ Language Standard to C++ 17
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_EXTENSIONS ON)

target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${IMGUI_SOURCES})
target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
#target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${ENKITS_SOURCES})

sort_into_source_group(ENGINE_HEADERS ${HEADER_DIR} "")
sort_into_source_group(ENGINE_SOURCES ${SOURCE_DIR} "")
sort_into_source_group(PLATFORM_HEADERS ${HEADER_DIR} "")
sort_into_source_group(PLATFORM_SOURCES ${SOURCE_DIR} "")
sort_into_source_group(EDITOR_HEADERS ${HEADER_DIR} "")
sort_into_source_group(EDITOR_SOURCES ${SOURCE_DIR} "")
sort_into_source_group(IMGUI_SOURCES ${IMGUI_DIR} imgui)
sort_into_source_group(OPENSIMPLEX_SOURCES ${OPENSIMPLEX_DIR} opensimplexnoise)
#sort_into_source_group(VKBOOTSTRAP_SOURCES ${VKBOOTSTRAP_DIR} vkbootstrap)
sort_into_source_group(ENKITS_SOURCES ${ENKITS_DIR} enkits)

message("")

set(PLATFORM ${CMAKE_HOST_SYSTEM_NAME})

set(PLATFORM_WINDOWS FALSE)
set(PLATFORM_LINUX FALSE)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
endif()

# Get & Build Required Libraries
find_package(Vulkan 1.4.309 REQUIRED)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# tomlplusplus
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

# raylib
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG c1ab645
        GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

# raylib-cpp
# find_package(raylib_cpp QUIET)
if (NOT raylib_cpp_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib_cpp
        GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
        GIT_TAG 6d9d02c
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(raylib_cpp)
endif()

target_link_libraries(${PUFFIN_ENGINE_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PUFFIN_ENGINE_NAME} nlohmann_json::nlohmann_json)
target_link_libraries(${PUFFIN_ENGINE_NAME} tomlplusplus::tomlplusplus)
target_link_libraries(${PUFFIN_ENGINE_NAME} raylib)
target_link_libraries(${PUFFIN_ENGINE_NAME} raylib_cpp)

if (PLATFORM_WINDOWS)
	set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/win64)
	set(LIB_SUFFIX .lib)
	set(DLL_SUFFIX .dll)
elseif (PLATFORM_LINUX)
	set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux)
	set(LIB_SUFFIX .a)
	set(DLL_SUFFIX .so)
endif()

if (PLATFORM_WINDOWS)

	# Define Debug Libs
	#target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/glfw/Debug/glfw3${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/lz4/Debug/liblz4_static${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/compressonator/Compressonator_MTd${LIB_SUFFIX})

	# Define Release Libs
	#target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/glfw/Release/glfw3${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/lz4/Release/liblz4_static${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/compressonator/Compressonator_MT${LIB_SUFFIX})

	#target_link_libraries(${PUFFIN_ENGINE_NAME} winmm.lib)

elseif (PLATFORM_LINUX)

	find_package(glfw3 3.4 REQUIRED)

	target_link_libraries(${PUFFIN_ENGINE_NAME} glfw)
	target_link_libraries(${PUFFIN_ENGINE_NAME} /usr/local/lib/liblz4${LIB_SUFFIX})

endif()

if (PLATFORM_WINDOWS AND BOX2D_PHYSICS_SUPPORT)
	target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/box2d/Debug/box2dd${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/box2d/Release/box2d${LIB_SUFFIX})
endif()

if (PLATFORM_WINDOWS AND JOLT_PHYSICS_SUPPORT)
	if (DOUBLE_PRECISION_SUPPORT)
		target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt_Double${LIB_SUFFIX})
		target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt_Double${LIB_SUFFIX})
	else ()
		target_link_libraries(${PUFFIN_ENGINE_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt${LIB_SUFFIX})
		target_link_libraries(${PUFFIN_ENGINE_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt${LIB_SUFFIX})
	endif ()
endif()

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Add include paths
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC include)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vma)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/box2d/include)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/OpenSimplexNoise)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/imgui)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/imgui-filebrowser)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/lz4/lib)
#target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/vma-hpp/include)
#target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/vkb/src)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/tinygltf)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/enkits/src)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/entt/src)
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${DEPS_DIR}/jolt)

target_link_libraries(${PUFFIN_EDITOR_NAME} ${PUFFIN_ENGINE_NAME})

target_compile_features(${PUFFIN_ENGINE_NAME} PRIVATE cxx_std_17)
target_compile_features(${PUFFIN_EDITOR_NAME} PRIVATE cxx_std_17)

set_property(TARGET ${PUFFIN_ENGINE_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(TARGET ${PUFFIN_EDITOR_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PUFFIN_EDITOR_NAME})

if (PFN_PLATFORM STREQUAL "Raylib")

	target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC SUPPORT_CUSTOM_FRAME_CONTROL 1)

endif()

# Add definitions for supported functionality
if (DOUBLE_PRECISION_SUPPORT)
	target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_DOUBLE_PRECISION)
endif()

if (PLATFORM_WINDOWS AND BOX2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_BOX2D_PHYSICS)
endif()

if (PLATFORM_WINDOWS AND JOLT_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_JOLT_PHYSICS)
endif()

if (ONAGER2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_ONAGER2D_PHYSICS)
endif()

target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC GLM_FORCE_SIMD_AVX2)
target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC NOMINMAX)
#target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC VK_NO_PROTOTYPES)

# Platform defines
if (PLATFORM_WINDOWS)
    target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_PLATFORM_WIN32)
    #target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    #target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC GLFW_EXPOSE_NATIVE_WIN32)
elseif (PLATFORM_LINUX)
    target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC PFN_PLATFORM_LINUX)
    #target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC VK_USE_PLATFORM_XLIB_KHR)
    #target_compile_definitions(${PUFFIN_ENGINE_NAME} PUBLIC GLFW_EXPOSE_NATIVE_X11)
endif()

if (MSVC)
	# Big Object Support (MSCV/Linux)
	target_compile_options(${PUFFIN_ENGINE_NAME} PUBLIC /bigobj)
	target_compile_options(${PUFFIN_ENGINE_NAME} PUBLIC /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PUFFIN_ENGINE_NAME} PUBLIC /MP)
	# Enabled updated __cplusplus macro
	target_compile_options(${PUFFIN_ENGINE_NAME} PUBLIC "/Zc:__cplusplus")

	# Big Object Support (MSCV/Linux)
	target_compile_options(${PUFFIN_EDITOR_NAME} PUBLIC /bigobj)
	target_compile_options(${PUFFIN_EDITOR_NAME} PUBLIC /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PUFFIN_EDITOR_NAME} PUBLIC /MP)
	# Enabled updated __cplusplus macro
	target_compile_options(${PUFFIN_EDITOR_NAME} PUBLIC "/Zc:__cplusplus")
else ()
	#target_compile_options(${PUFFIN_ENGINE_NAME} PUBLIC -Wa, -mbig-obj)
endif ()
