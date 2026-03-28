# This file configures the "build" of this SKSE plugin
# - how to compile the C++ plugin (in addition to settings found in CMakePresets.json)
# - where to output the resulting .dll file

# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.20)

# [BEGIN set_project_name]
# Set your project name. This will be the name of your SKSE .dll file.
project(DynamicInteriorFogRemover VERSION 0.0.1 LANGUAGES CXX)

# [END set_project_name]

# #
# YOU DO NOT NEED TO EDIT ANYTHING BELOW HERE
# #

# If you're not using a mod manager, you probably want the SKSE plugin to go
# inside of your Skyrim "Data" folder.
#
# To do this automatically, set the `SKYRIM_FOLDER` environment variable
# to the path of your Skyrim Special Edition folder
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

# If you're using Mod Organizer 2 or Vortex, you might want this to go inside
# of your "mods" folder, inside of a subfolder named "<your mod>".
#
# To do this automatically, set the `SKYRIM_MODS_FOLDER` environment variable
# to the path of your "mods" folder
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()

# Otherwise, you can set OUTPUT_FOLDER to any place you'd like :)
# Only set the fallback path if OUTPUT_FOLDER hasn't been set by an environment variable above.
# This prevents CI from trying to copy to a local path that doesn't exist on the runner.
if(NOT DEFINED OUTPUT_FOLDER)
    set(OUTPUT_FOLDER "C:/semods/${PROJECT_NAME}")
endif()

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES src/plugin.cpp src/Config.cpp src/FogManager.cpp src/Hooks.cpp src/UI.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h) # <--- PCH.h is required!

# Force Unicode everywhere
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure OUTPUT_FOLDER above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # Copy static assets
    set(STATIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/static")
    set(STATIC_DST "${OUTPUT_FOLDER}/SKSE/Plugins/${PROJECT_NAME}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${STATIC_DST}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${STATIC_SRC}" "${STATIC_DST}"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()

# Release zip is always produced on Release builds, regardless of OUTPUT_FOLDER.
# This ensures CI can still create the zip for GitHub releases even without a local mod folder.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(STATIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/static")
    set(RELEASE_DST "${CMAKE_CURRENT_SOURCE_DIR}/release")
    set(RELEASE_STATIC_DST "${RELEASE_DST}/SKSE/Plugins/${PROJECT_NAME}")
    set(RELEASE_DLL_DST "${RELEASE_DST}/SKSE/Plugins")
    set(RELEASE_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-release.zip")

    file(MAKE_DIRECTORY "${RELEASE_DST}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RELEASE_DST}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RELEASE_STATIC_DST}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RELEASE_DLL_DST}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${STATIC_SRC}" "${RELEASE_STATIC_DST}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${RELEASE_DLL_DST}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "${RELEASE_ZIP}"
        COMMAND "${CMAKE_COMMAND}" -E chdir "${RELEASE_DST}"
            "${CMAKE_COMMAND}" -E tar "cfv" "${RELEASE_ZIP}" --format=zip "."
        VERBATIM
    )
endif()