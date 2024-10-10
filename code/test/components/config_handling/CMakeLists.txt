FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    REQUIRES unity json jomjol_helper
                    EMBED_TXTFILES ${CMAKE_CURRENT_SOURCE_DIR}/config_json_default_expected.txt
                                    config_json_high_limit_expected.txt
                                    config_json_high_limit.txt
                                    config_json_low_limit_expected.txt
                                    config_json_low_limit.txt
                                    config_json_special_expected.txt
                                    config_json_special.txt)
