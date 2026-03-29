cmake_minimum_required(VERSION 3.20)

# [BEGIN set_project_name]
project(DynamicInteriorFog VERSION 0.0.3 LANGUAGES CXX)
# [END set_project_name]

find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES src/plugin.cpp src/Config.cpp src/FogManager.cpp src/Hooks.cpp src/UI.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h)

target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Debug: always output to C:/semods/{PROJECT_NAME}
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OUTPUT_FOLDER "C:/semods/${PROJECT_NAME}")
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

    # Copy .pdb debug symbols
    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

# Release: only create the zip, no local output
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