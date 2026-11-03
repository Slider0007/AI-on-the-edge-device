# ##################################################################
# Extract GIT infos and set variables in version.cpp
# ##################################################################

# Look for git executable
find_program(GIT_EXECUTABLE NAMES git)

string(TIMESTAMP BUILD_TIME "%Y-%m-%dT%H:%M:%S+0000" UTC)

if(NOT GIT_EXECUTABLE)
    message(STATUS "Git not found. Using default version placeholders")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
else()
    # Resolve the actual .git directory (supports worktrees where .git is a file)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DIR_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(GIT_DIR_PATH)
        if(NOT IS_ABSOLUTE "${GIT_DIR_PATH}")
            set(GIT_DIR_PATH "${CMAKE_SOURCE_DIR}/${GIT_DIR_PATH}")
        endif()

        # Core Git metadata files to track
        set(GIT_DEPENDENCIES
            "${GIT_DIR_PATH}/HEAD"
            "${GIT_DIR_PATH}/index"
        )

        # Track the active branch reference file (e.g., .git/refs/heads/main)
        # so commits on the current branch correctly trigger a re-configure.
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --symbolic-full-name HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REF
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(GIT_REF AND EXISTS "${GIT_DIR_PATH}/${GIT_REF}")
            list(APPEND GIT_DEPENDENCIES "${GIT_DIR_PATH}/${GIT_REF}")
        endif()

        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GIT_DEPENDENCIES})
    endif()

    # Use ${GIT_EXECUTABLE} instead of raw "git"
    execute_process(COMMAND ${GIT_EXECUTABLE} log --pretty=format:%h --abbrev=7 -n 1
                    OUTPUT_VARIABLE GIT_REV
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)

    if (GIT_REV STREQUAL "")
        set(GIT_TAG "N/A")
        set(GIT_BRANCH "N/A")
        set(GIT_REV "N/A")
        set(GIT_DIFF "")
    else()
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags
                        OUTPUT_VARIABLE GIT_TAG
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET)
        if (GIT_TAG STREQUAL "")
            set(GIT_TAG "N/A")
        endif()

        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                        OUTPUT_VARIABLE GIT_BRANCH
                        OUTPUT_STRIP_TRAILING_WHITESPACE)

        if (GIT_BRANCH STREQUAL "")
            set(GIT_BRANCH "N/A")
        endif()

        execute_process(COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
                        RESULT_VARIABLE GIT_DIFF_RES
                        ERROR_QUIET)

        execute_process(COMMAND ${GIT_EXECUTABLE} ls-files --others --exclude-standard "*.cpp" "*.c" "*.h" "*.hpp"
                        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                        OUTPUT_VARIABLE GIT_UNTRACKED_SOURCES
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET)

        if(NOT GIT_DIFF_RES EQUAL 0 OR NOT GIT_UNTRACKED_SOURCES STREQUAL "")
            set(GIT_DIFF "+")
        else()
            set(GIT_DIFF "")
        endif()
    endif()
endif()

# Append dirty flag / tags
if (GIT_TAG STREQUAL "N/A")
    set(VERSION_TAG "N/A")
    set(VERSION_HTML "Develop: ${GIT_BRANCH} @ ${GIT_REV}${GIT_DIFF}")
else()
    set(VERSION_TAG "${GIT_TAG}${GIT_DIFF}")
    set(VERSION_HTML "${GIT_TAG}${GIT_DIFF}")
endif()

set(VERSION "const char* GIT_TAG=\"${VERSION_TAG}\";
const char* GIT_BRANCH=\"${GIT_BRANCH}\";
const char* GIT_REV=\"${GIT_REV}${GIT_DIFF}\";
const char* BUILD_TIME=\"${BUILD_TIME}\";"
)

# Write version.cpp only if content has changed
set(VERSION_FILE "${CMAKE_SOURCE_DIR}/main/version.cpp")
if(EXISTS "${VERSION_FILE}")
    file(READ "${VERSION_FILE}" VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT VERSION STREQUAL VERSION_)
    file(WRITE "${VERSION_FILE}" "${VERSION}")

    # Ensure target directory exists before writing external version file
    get_filename_component(SD_HTML_DIR "${CMAKE_SOURCE_DIR}/../sd-card/html" ABSOLUTE)
    file(MAKE_DIRECTORY "${SD_HTML_DIR}")
    file(WRITE "${SD_HTML_DIR}/version.txt" "${VERSION_HTML}")
endif()

# #######################################################################
# Component Registration
# #######################################################################

FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_sdmmc nvs_flash esp_psram esp_netif esp_hw_support config_handling camera_ctrl
                             logfile_handling mainprocess_ctrl network_ctrl webserver_softap misc_helper openmetrics_ctrl)