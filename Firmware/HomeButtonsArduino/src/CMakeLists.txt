# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# idf_component_register(SRCS ${app_sources})

message(STATUS "HOME_BUTTONS_ORIGINAL: ${HOME_BUTTONS_ORIGINAL}")
message(STATUS "HOME_BUTTONS_MINI: ${HOME_BUTTONS_MINI}")
message(STATUS "HOME_BUTTONS_PRO: ${HOME_BUTTONS_PRO}")
message(STATUS "HOME_BUTTONS_INDUSTRIAL: ${HOME_BUTTONS_INDUSTRIAL}")

# Get list of all .c and .cpp files in the src directory
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.cpp)

if(DEFINED HOME_BUTTONS_ORIGINAL OR DEFINED HOME_BUTTONS_MINI)
    file(GLOB_RECURSE files_to_remove "${CMAKE_SOURCE_DIR}/src/touch/*")

    foreach(file ${files_to_remove})
        list(REMOVE_ITEM app_sources "${file}")
    endforeach()
endif()

if(DEFINED HOME_BUTTONS_PRO)
    file(GLOB_RECURSE files_to_remove "${CMAKE_SOURCE_DIR}/src/button_ui/*")

    foreach(file ${files_to_remove})
        list(REMOVE_ITEM app_sources "${file}")
    endforeach()
endif()

if(DEFINED HOME_BUTTONS_INDUSTRIAL)
    file(GLOB_RECURSE files_to_remove
        "${CMAKE_SOURCE_DIR}/src/display/*"
        "${CMAKE_SOURCE_DIR}/src/mdi/*"
        "${CMAKE_SOURCE_DIR}/src/touch/*")

    foreach(file ${files_to_remove})
        list(REMOVE_ITEM app_sources "${file}")
    endforeach()
endif()

foreach(src_file ${app_sources})
    message(STATUS "Source file: ${src_file}")
endforeach()

idf_component_register(SRCS ${app_sources})
