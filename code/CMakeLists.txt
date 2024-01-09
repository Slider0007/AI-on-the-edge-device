cmake_minimum_required(VERSION 3.16.0)

if(EXISTS "${SDKCONFIG}.defaults")
    if(EXISTS "sdkconfig.defaults")
        set(SDKCONFIG_DEFAULTS "${SDKCONFIG}.defaults;sdkconfig.defaults")
        message(STATUS "-- Using defaults: ${SDKCONFIG_DEFAULTS} + sdkconfig.defaults")
    else()
        set(SDKCONFIG_DEFAULTS "${SDKCONFIG}.defaults")
    endif()
    message(STATUS "-- Using defaults: ${SDKCONFIG_DEFAULTS}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(AI-on-the-edge)
