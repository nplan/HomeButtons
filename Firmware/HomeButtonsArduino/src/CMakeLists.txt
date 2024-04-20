# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# idf_component_register(SRCS ${app_sources})

message(STATUS "HOME_BUTTONS_ORIGINAL: ${HOME_BUTTONS_ORIGINAL}")
message(STATUS "HOME_BUTTONS_MINI: ${HOME_BUTTONS_MINI}")
message(STATUS "HOME_BUTTONS_PRO: ${HOME_BUTTONS_PRO}")

# Get list of all .c and .cpp files in the src directory
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.cpp)

if(NOT DEFINED HOME_BUTTONS_ORIGINAL AND NOT DEFINED HOME_BUTTONS_MINI)
    file(GLOB_RECURSE files_to_remove "${CMAKE_SOURCE_DIR}/src/original_mini/*")

    foreach(file ${files_to_remove})
        list(REMOVE_ITEM app_sources "${file}")
    endforeach()
endif()

if(NOT DEFINED HOME_BUTTONS_PRO)
    file(GLOB_RECURSE files_to_remove "${CMAKE_SOURCE_DIR}/src/pro/*")

    foreach(file ${files_to_remove})
        list(REMOVE_ITEM app_sources "${file}")
    endforeach()
endif()

foreach(src_file ${app_sources})
    message(STATUS "Source file: ${src_file}")
endforeach()

idf_component_register(SRCS ${app_sources})
