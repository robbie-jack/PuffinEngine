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
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(EDITOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/editor)
set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
#set(VKBOOTSTRAP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/vkb/src)
set(OPENSIMPLEX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenSimplexNoise/OpenSimplexNoise)
set(ENKITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/enkits)

file(GLOB_RECURSE ENGINE_HEADERS
	"${HEADER_DIR}/puffin/*.h"
)

file(GLOB_RECURSE ENGINE_SOURCES
	"${SOURCE_DIR}/puffin/*.cpp"
)

file(GLOB_RECURSE EDITOR_HEADERS
	"${EDITOR_DIR}/*.h"
)

file(GLOB_RECURSE EDITOR_SOURCES
	"${EDITOR_DIR}/*.cpp"
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

set (PLATFORM_HEADERS ${PLATFORM_DIR}/platform.h)
set (PLATFORM_SOURCES ${PLATFORM_DIR}/platform.cpp)

if (PFN_PLATFORM STREQUAL "Raylib")

	message("Puffin Platform: Raylib")

	set(CUSTOMIZE_BUILD ON)
	set(SUPPORT_CUSTOM_FRAME_CONTROL 1)

	file(GLOB_RECURSE RAYLIB_PLATFORM_HEADERS
		"${PLATFORM_DIR}/raylib/*.h"
	)

	file(GLOB_RECURSE RAYLIB_PLATFORM_SOURCES
		"${PLATFORM_DIR}/raylib/*.cpp"
	)

	set (PLATFORM_HEADERS ${PLATFORM_HEADERS} ${RAYLIB_PLATFORM_HEADERS})
	set (PLATFORM_SOURCES ${PLATFORM_SOURCES} ${RAYLIB_PLATFORM_SOURCES})

else()

	message("Puffin Platform: None")

endif()

project ("Puffin" DESCRIPTION "3D ECS Game Engine" LANGUAGES CXX)
add_library(${PUFFIN_ENGINE_NAME} ${ENGINE_HEADERS} ${ENGINE_SOURCES} ${PLATFORM_HEADERS} ${PLATFORM_SOURCES})
add_executable(${PUFFIN_EDITOR_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES})

# Set C++ Language Standard to C++ 17
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_STANDARD 17)
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(${PUFFIN_ENGINE_NAME} PROPERTIES CMAKE_CXX_EXTENSIONS ON)

target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${IMGUI_SOURCES})
target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${OPENSIMPLEX_SOURCES})
#target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${VKBOOTSTRAP_SOURCES})
target_sources(${PUFFIN_ENGINE_NAME} PRIVATE ${ENKITS_SOURCES})

sort_into_source_group(ENGINE_HEADERS ${HEADER_DIR} engine)
sort_into_source_group(ENGINE_SOURCES ${SOURCE_DIR} engine)
sort_into_source_group(PLATFORM_HEADERS ${PLATFORM_DIR} platform)
sort_into_source_group(PLATFORM_SOURCES ${PLATFORM_DIR} platform)
sort_into_source_group(EDITOR_HEADERS ${EDITOR_DIR} "")
sort_into_source_group(EDITOR_SOURCES ${EDITOR_DIR} "")
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
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${ENGINE_DIR})
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${PLATFORM_DIR})
target_include_directories(${PUFFIN_ENGINE_NAME} PUBLIC ${THIRD_PARTY_DIR})
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

target_include_directories(${PUFFIN_EDITOR_NAME} PUBLIC ${EDITOR_DIR})

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
