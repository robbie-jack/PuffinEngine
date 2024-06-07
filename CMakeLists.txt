cmake_minimum_required(VERSION 3.22.1)

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

include(FetchContent)

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")

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

checkout_git_branch("deps/imgui" "docking")

# Define CMAKE Variables
set(PUFFIN_LIBRARY_NAME PuffinEngine)
set(PUFFIN_APP_NAME PuffinApp)

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
option(BOX2D_PHYSICS_SUPPORT "Compile with Box2D Physics Support" OFF)
option(JOLT_PHYSICS_SUPPORT "Compile with Jolt Physics Support" ON)
option(ONAGER2D_PHYSICS_SUPPORT "Compile with Onager 2D Physics Support" OFF)

set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
set(VKBOOTSTRAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/vkb/src)
set(ANGELSCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/angelscript)
set(OPENSIMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenSimplexNoise/OpenSimplexNoise)
set(ENKITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/enkits)

set(HEADERS
	# Root
	${HEADER_DIR}/puffin/manipulation_gizmo.h
	${HEADER_DIR}/puffin/math_helpers.h
	${HEADER_DIR}/puffin/project_settings.h
	# Assets
	${HEADER_DIR}/puffin/assets/asset.h
	${HEADER_DIR}/puffin/assets/asset_importers.h
	${HEADER_DIR}/puffin/assets/asset_registry.h
	${HEADER_DIR}/puffin/assets/material_asset.h
	${HEADER_DIR}/puffin/assets/mesh_asset.h
	${HEADER_DIR}/puffin/assets/shader_asset.h
	${HEADER_DIR}/puffin/assets/sound_asset.h
	${HEADER_DIR}/puffin/assets/texture_asset.h
	# Audio
	${HEADER_DIR}/puffin/audio/audio_subsystem.h
	${HEADER_DIR}/puffin/audio/mini_audio_subsystem.h
	# Components
	${HEADER_DIR}/puffin/components/transform_component_2d.h
	${HEADER_DIR}/puffin/components/transform_component_3d.h
	${HEADER_DIR}/puffin/components/physics/2d/rigidbody_component_2d.h
	${HEADER_DIR}/puffin/components/physics/2d/shape_components_2d.h
	${HEADER_DIR}/puffin/components/physics/2d/velocity_component_2d.h
	${HEADER_DIR}/puffin/components/physics/3d/rigidbody_component_3d.h
	${HEADER_DIR}/puffin/components/physics/3d/shape_components_3d.h
	${HEADER_DIR}/puffin/components/physics/3d/velocity_component_3d.h
	${HEADER_DIR}/puffin/components/procedural/procedural_mesh_component.h
	${HEADER_DIR}/puffin/components/rendering/camera_component.h
	${HEADER_DIR}/puffin/components/rendering/light_component.h
	${HEADER_DIR}/puffin/components/rendering/mesh_component.h
	${HEADER_DIR}/puffin/components/scripting/angelscript_component.h
	# ECS
	${HEADER_DIR}/puffin/ecs/entt_subsystem.h
	# Core
	${HEADER_DIR}/puffin/core/application.h
	${HEADER_DIR}/puffin/core/engine.h
	${HEADER_DIR}/puffin/core/enkits_subsystem.h
	${HEADER_DIR}/puffin/core/scene_subsystem.h
	${HEADER_DIR}/puffin/core/signal_subsystem.h
	${HEADER_DIR}/puffin/core/system.h
	# Input
	${HEADER_DIR}/puffin/input/input_event.h
	${HEADER_DIR}/puffin/input/input_subsystem.h
	# Nodes
	${HEADER_DIR}/puffin/nodes/node.h
	${HEADER_DIR}/puffin/nodes/transform_node_2d.h
	${HEADER_DIR}/puffin/nodes/transform_node_3d.h
	${HEADER_DIR}/puffin/nodes/physics/rigidbody_node_3d.h
	${HEADER_DIR}/puffin/nodes/rendering/camera_node_3d.h
	${HEADER_DIR}/puffin/nodes/rendering/light_node_3d.h
	${HEADER_DIR}/puffin/nodes/rendering/mesh_node.h
	# Physics
	${HEADER_DIR}/puffin/physics/body_type.h
	${HEADER_DIR}/puffin/physics/collision_event.h
	${HEADER_DIR}/puffin/physics/physics_constants.h
	${HEADER_DIR}/puffin/physics/onager2d/onager_physics_system_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/physics_helpers_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/physics_types_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/broadphase_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/sweep_and_prune_broadphase.h
	${HEADER_DIR}/puffin/physics/onager2d/broadphases/spatial_hash_broadphase_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/collider_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/polygon_collider_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/box_collider_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/colliders/circle_collider_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/shape_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/polygon_shape_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/circle_shape_2d.h
	${HEADER_DIR}/puffin/physics/onager2d/shapes/box_shape_2d.h
	${HEADER_DIR}/puffin/physics/box2d/box2d_contact_listener.h
	${HEADER_DIR}/puffin/physics/box2d/box2d_physics_system.h
	${HEADER_DIR}/puffin/physics/jolt/jolt_physics_system.h
	${HEADER_DIR}/puffin/physics/jolt/jolt_physics_types.h
	# Procedural
	${HEADER_DIR}/puffin/procedural/procedural_mesh_gen_system.h
	# Rendering
	${HEADER_DIR}/puffin/rendering/camera_subsystem.h
	${HEADER_DIR}/puffin/rendering/material_globals.h
	${HEADER_DIR}/puffin/rendering/render_globals.h
	${HEADER_DIR}/puffin/rendering/render_helpers.h
	# Rendering Vulkan
	${HEADER_DIR}/puffin/rendering/vulkan/descriptors_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/helpers_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/material_registry_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/pipeline_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/render_graph_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/render_system_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/resource_manager_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/types_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/unified_geometry_buffer_vk.h
	${HEADER_DIR}/puffin/rendering/vulkan/vertex_vk.h
	# Scene
	${HEADER_DIR}/puffin/scene/scene_graph.h
	# Scripting
	${HEADER_DIR}/puffin/scripting/angelscript/angelscript_engine_interface.h
	${HEADER_DIR}/puffin/scripting/angelscript/angelscript_system.h
	${HEADER_DIR}/puffin/scripting/angelscript/register_type_helpers.h
	# Types
	${HEADER_DIR}/puffin/types/aabb.h
	${HEADER_DIR}/puffin/types/component_flags.h
	${HEADER_DIR}/puffin/types/deletion_queue.h
	${HEADER_DIR}/puffin/types/euler_angles.h
	${HEADER_DIR}/puffin/types/matrix.h
	${HEADER_DIR}/puffin/types/packed_array.h
	${HEADER_DIR}/puffin/types/packed_vector.h
	${HEADER_DIR}/puffin/types/quat.h
	${HEADER_DIR}/puffin/types/ring_buffer.h
	${HEADER_DIR}/puffin/types/uuid.h
	${HEADER_DIR}/puffin/types/vector.h
	${HEADER_DIR}/puffin/types/vertex.h
	# UI
	${HEADER_DIR}/puffin/ui/editor/ui_subsystem.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_content_browser.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window_node_editor.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window_performance.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window_scene_hierarchy.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window_settings.h
	${HEADER_DIR}/puffin/ui/editor/windows/ui_window_viewport.h
	# Window
	${HEADER_DIR}/puffin/window/window_subsystem.h
	)

set(SOURCES 
	# Root 
	${SOURCE_DIR}/puffin/manipulation_gizmo.cpp
	# Assets
	${SOURCE_DIR}/puffin/assets/asset_importers.cpp
	${SOURCE_DIR}/puffin/assets/asset_registry.cpp
	${SOURCE_DIR}/puffin/assets/material_asset.cpp
	${SOURCE_DIR}/puffin/assets/mesh_asset.cpp
	${SOURCE_DIR}/puffin/assets/shader_asset.cpp
	${SOURCE_DIR}/puffin/assets/texture_asset.cpp
	# Audio
	${SOURCE_DIR}/puffin/audio/audio_subsystem.cpp
	${SOURCE_DIR}/puffin/audio/mini_audio_subsystem.cpp
	# Engine
	${SOURCE_DIR}/puffin/core/application.cpp
	${SOURCE_DIR}/puffin/core/engine.cpp
	${SOURCE_DIR}/puffin/core/enkits_subsystem.cpp
	# Input
	${SOURCE_DIR}/puffin/input/input_subsystem.cpp
	# Nodes
	${SOURCE_DIR}/puffin/nodes/node.cpp
	${SOURCE_DIR}/puffin/nodes/transform_node_2d.cpp
	${SOURCE_DIR}/puffin/nodes/transform_node_3d.cpp
	${SOURCE_DIR}/puffin/nodes/physics/rigidbody_node_3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/camera_node_3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/light_node_3d.cpp
	${SOURCE_DIR}/puffin/nodes/rendering/mesh_node.cpp
	# Physics
	${SOURCE_DIR}/puffin/physics/onager2d/onager_physics_system_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/shapes/circle_shape_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/shapes/box_shape_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/box_collider_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/circle_collider_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/colliders/polygon_collider_2d.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/broadphases/sweep_and_prune_broadphase.cpp
	${SOURCE_DIR}/puffin/physics/onager2d/broadphases/spatial_hash_broadphase_2d.cpp
	${SOURCE_DIR}/puffin/physics/box2d/box2d_contact_listener.cpp
	${SOURCE_DIR}/puffin/physics/box2d/box2d_physics_system.cpp
	${SOURCE_DIR}/puffin/physics/jolt/jolt_physics_system.cpp
	# Procedural
	${SOURCE_DIR}/puffin/procedural/procedural_mesh_gen_system.cpp
	# Rendering
	${SOURCE_DIR}/puffin/rendering/camera_subsystem.cpp
	${SOURCE_DIR}/puffin/rendering/render_helpers.cpp
	# Rendering Vulkan
	${SOURCE_DIR}/puffin/rendering/vulkan/descriptors_vk.cpp
	${SOURCE_DIR}/puffin/rendering/vulkan/helpers_vk.cpp
	${SOURCE_DIR}/puffin/rendering/vulkan/material_registry_vk.cpp
	${SOURCE_DIR}/puffin/rendering/vulkan/render_system_vk.cpp
	${SOURCE_DIR}/puffin/rendering/vulkan/resource_manager_vk.cpp
	${SOURCE_DIR}/puffin/rendering/vulkan/unified_geometry_buffer_vk.cpp
	# Scene
	${SOURCE_DIR}/puffin/scene/scene_graph.cpp
	# Scripting
	${SOURCE_DIR}/puffin/scripting/angelscript/angelscript_system.cpp
	${SOURCE_DIR}/puffin/scripting/angelscript/angelscript_engine_interface.cpp
	# UI
	${SOURCE_DIR}/puffin/ui/editor/ui_subsystem.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_content_browser.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window_node_editor.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window_performance.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window_scene_hierarchy.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window_settings.cpp
	${SOURCE_DIR}/puffin/ui/editor/windows/ui_window_viewport.cpp
	# Window
	${SOURCE_DIR}/puffin/window/window_subsystem.cpp
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
	
set (ENKITS_SOURCES
	${ENKITS_DIR}/src/TaskScheduler.h
	${ENKITS_DIR}/src/TaskScheduler.cpp
	)

add_library(${PUFFIN_LIBRARY_NAME} ${HEADERS} ${SOURCES})

project (${PUFFIN_APP_NAME} DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)
add_executable(${PUFFIN_APP_NAME} ${SOURCE_DIR}/main.cpp )

# Set C++ Language Standard to C++ 17
set_target_properties(${PUFFIN_LIBRARY_NAME} PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(${PUFFIN_LIBRARY_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PUFFIN_LIBRARY_NAME} PROPERTIES CMAKE_CXX_EXTENSIONS ON)

target_sources(${PUFFIN_LIBRARY_NAME} PRIVATE ${IMGUI_SOURCES})
target_sources(${PUFFIN_LIBRARY_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
target_sources(${PUFFIN_LIBRARY_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PUFFIN_LIBRARY_NAME} PRIVATE ${ANGELSCRIPT_SOURCES})
target_sources(${PUFFIN_LIBRARY_NAME} PRIVATE ${ENKITS_SOURCES})

message("")
message("============================================================")
message("Sorting Source Files into relative Subfolders")
message("============================================================")

sort_into_source_group(HEADERS ${HEADER_DIR} "")
sort_into_source_group(SOURCES ${SOURCE_DIR} "")
sort_into_source_group(IMGUI_SOURCES ${IMGUI_DIR} imgui)
sort_into_source_group(OPENSIMPLEX_SOURCES ${OPENSIMPLEX_DIR} opensimplexnoise)
sort_into_source_group(VKBOOTSTRAP_SOURCES ${VKBOOTSTRAP_DIR} vkbootstrap)
sort_into_source_group(ANGELSCRIPT_SOURCES ${ANGELSCRIPT_DIR} angelscript)
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
find_package(Vulkan REQUIRED)
CPMAddPackage("gh:nlohmann/json@3.11.2")

target_link_libraries(${PUFFIN_LIBRARY_NAME} ${Vulkan_LIBRARIES})
target_link_libraries(${PUFFIN_LIBRARY_NAME} nlohmann_json::nlohmann_json)

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
	target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/glfw/Debug/glfw3${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/angelscript/Debug/angelscript64d${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/lz4/Debug/liblz4_static${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/compressonator/Compressonator_MTd${LIB_SUFFIX})

	# Define Release Libs
	target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/glfw/Release/glfw3${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/angelscript/Release/angelscript64${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/lz4/Release/liblz4_static${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/compressonator/Compressonator_MT${LIB_SUFFIX})

elseif (PLATFORM_LINUX)

	find_package(glfw3 3.4 REQUIRED)

	target_link_libraries(${PUFFIN_LIBRARY_NAME} glfw)
	target_link_libraries(${PUFFIN_LIBRARY_NAME} usr/local/lib/libangelscript${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} usr/local/lib/lz4${LIB_SUFFIX})

endif()

if (PLATFORM_WINDOWS AND BOX2D_PHYSICS_SUPPORT)
	target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/box2d/Debug/box2d${LIB_SUFFIX})
	target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/box2d/Release/box2d${LIB_SUFFIX})
endif()

if (PLATFORM_WINDOWS AND JOLT_PHYSICS_SUPPORT)
	if (DOUBLE_PRECISION_SUPPORT)
		target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt_Double${LIB_SUFFIX})
		target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt_Double${LIB_SUFFIX})
	else ()
		target_link_libraries(${PUFFIN_LIBRARY_NAME} debug ${LIB_DIR}/jolt/Debug/Jolt${LIB_SUFFIX})
		target_link_libraries(${PUFFIN_LIBRARY_NAME} optimized ${LIB_DIR}/jolt/Release/Jolt${LIB_SUFFIX})
	endif ()
endif()

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

# Add include paths
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC include)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC include/angelscript)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC $ENV{VULKAN_SDK}/include)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC $ENV{VULKAN_SDK}/include/vma)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/box2d/include)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/OpenSimplexNoise)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/imgui)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/imgui-filebrowser)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/lz4/lib)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/vma-hpp/include)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/vkb/src)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/tinygltf)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/enkits/src)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/entt/src)
target_include_directories(${PUFFIN_LIBRARY_NAME} PUBLIC ${DEPS_DIR}/jolt)

target_link_libraries(${PUFFIN_APP_NAME} ${PUFFIN_LIBRARY_NAME})

target_compile_features(${PUFFIN_LIBRARY_NAME} PRIVATE cxx_std_17)
target_compile_features(${PUFFIN_APP_NAME} PRIVATE cxx_std_17)

set_property(TARGET ${PUFFIN_LIBRARY_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_property(TARGET ${PUFFIN_APP_NAME} PROPERTY 
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PUFFIN_APP_NAME})

# Add definitions for supported functionality
if (DOUBLE_PRECISION_SUPPORT)
	target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_DOUBLE_PRECISION)
endif()

if (PLATFORM_WINDOWS AND BOX2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_BOX2D_PHYSICS)
endif()

if (PLATFORM_WINDOWS AND JOLT_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_JOLT_PHYSICS)
endif()

if (ONAGER2D_PHYSICS_SUPPORT)
	target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_ONAGER2D_PHYSICS)
endif()

target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC GLM_FORCE_SIMD_AVX2)
target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC NOMINMAX)

# Platform defines
if (PLATFORM_WINDOWS)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_PLATFORM_WIN32)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC GLFW_EXPOSE_NATIVE_WIN32)
elseif (PLATFORM_LINUX)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC PFN_PLATFORM_LINUX)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC VK_USE_PLATFORM_XLIB_KHR)
    target_compile_definitions(${PUFFIN_LIBRARY_NAME} PUBLIC GLFW_EXPOSE_NATIVE_X11)
endif()

if (MSVC)
	# Big Object Support (MSCV/Linux)
	target_compile_options(${PUFFIN_LIBRARY_NAME} PUBLIC /bigobj)
	target_compile_options(${PUFFIN_LIBRARY_NAME} PUBLIC /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PUFFIN_LIBRARY_NAME} PUBLIC /MP)
	# Enabled updated __cplusplus macro
	target_compile_options(${PUFFIN_LIBRARY_NAME} PUBLIC "/Zc:__cplusplus")

	# Big Object Support (MSCV/Linux)
	target_compile_options(${PUFFIN_APP_NAME} PUBLIC /bigobj)
	target_compile_options(${PUFFIN_APP_NAME} PUBLIC /arch:AVX2)
	# Multiprocessor (Multithreaded) Compilation Support
	target_compile_options(${PUFFIN_APP_NAME} PUBLIC /MP)
	# Enabled updated __cplusplus macro
	target_compile_options(${PUFFIN_APP_NAME} PUBLIC "/Zc:__cplusplus")
else ()
	#target_compile_options(${PUFFIN_LIBRARY_NAME} PUBLIC -Wa, -mbig-obj)
endif ()
