# Define Add Submodule Function
function(add_git_submodule dir)

find_package(Git REQUIRED)

#execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
    #WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    #COMMAND_ERROR_IS_FATAL ANY)

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

function(sort_into_source_group files dir sort_dir)

foreach(file IN LISTS ${files})
	#message("")
	#message("Generating Source Group for " ${file})
	file(RELATIVE_PATH relative_path ${dir} ${file})
	get_filename_component(relative_dir ${relative_path} DIRECTORY)
	#message("Relative Path: " ${relative_path})
	#message("Relative Dir: " ${relative_dir})
    string(REPLACE "/" "\\" relative_path_msvc "${relative_dir}")
    source_group("${sort_dir}/${relative_path_msvc}" FILES "${file}")
endforeach()

endfunction(sort_into_source_group)