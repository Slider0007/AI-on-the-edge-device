FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES misc_helper mainprocess_ctrl network_ctrl)


