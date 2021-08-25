cmake_minimum_required(VERSION 3.14)

project(RaychelLogger)

add_library(${PROJECT_NAME} SHARED src/Logger.cpp)

add_executable(Logger_test src/tests/main.cpp)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    RAYCHELLOGGER_EXPORTS
)

target_include_directories( ${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

#General compiler flags
if(${MSVC})
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 
        /wd4068 #unknown pragma
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wshadow 
        -Wno-unknown-pragmas
        -Wconversion
        -Werror
        -pedantic
    )
endif()

target_link_libraries(Logger_test PUBLIC RaychelLogger)

#Gimme that C++20 goodness
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED TRUE)