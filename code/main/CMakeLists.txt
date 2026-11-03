# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

##################################################################
# Extract GIT infos and set variables in version.cpp
##################################################################
execute_process(COMMAND git log --pretty=format:%h --abbrev=7 -n 1
                OUTPUT_VARIABLE GIT_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)

string(TIMESTAMP BUILD_TIME "%Y-%m-%dT%H:%M:%S+0000" UTC)

# Check whether we got any revision (which isn't
# always the case, e.g. when someone downloaded a zip
# file from Github instead of a checkout)
if ("${GIT_REV}" STREQUAL "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
else()
    execute_process(COMMAND git describe --exact-match --tags
                    OUTPUT_VARIABLE GIT_TAG
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if ("${GIT_TAG}" STREQUAL "")
        set(GIT_TAG "N/A")
    endif()

    execute_process(COMMAND git rev-parse --abbrev-ref HEAD
                    OUTPUT_VARIABLE GIT_BRANCH
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    if ("${GIT_BRANCH}" STREQUAL "")
        set(GIT_BRANCH "N/A")
    endif()

    # Check for tracked dirty changes
    execute_process(COMMAND git diff-index --quiet HEAD --
                    RESULT_VARIABLE GIT_DIFF_RES
                    ERROR_QUIET)

    # Check for untracked, non-ignored build inputs (*.cpp, *.c, *.h, *.hpp)
    execute_process(COMMAND git ls-files --others --exclude-standard "*.cpp" "*.c" "*.h" "*.hpp"
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE GIT_UNTRACKED_SOURCES
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)

    # Mark build dirty if either tracked modifications or untracked build inputs exist
    if(GIT_DIFF_RES EQUAL 1 OR NOT "${GIT_UNTRACKED_SOURCES}" STREQUAL "")
        set(GIT_DIFF "+")
    else()
        set(GIT_DIFF "")
    endif()
endif()

# Append dirty flag
if ("${GIT_TAG}" STREQUAL "N/A")
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

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE "${VERSION_FILE}" "${VERSION}")
    file(WRITE "${CMAKE_SOURCE_DIR}/../sd-card/html/version.txt" "${VERSION_HTML}")
endif()
#######################################################################
#######################################################################

FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_sdmmc nvs_flash esp_psram esp_netif esp_hw_support config_handling camera_ctrl
                        logfile_handling mainprocess_ctrl network_ctrl webserver_softap misc_helper openmetrics_ctrl)
