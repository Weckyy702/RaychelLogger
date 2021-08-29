set(RAYCHELLOGGER_HEADER_PATH "${RaychelLogger_SOURCE_DIR}/include/RaychelLogger")
set(RAYCHELLOGGER_HEADERS 
    ${RAYCHELLOGGER_HEADER_PATH}/Helper.h
    ${RAYCHELLOGGER_HEADER_PATH}/Logger.h
)

set(RAYCHELLOGGER_SOURCES
    Logger.cpp
)

add_library(RaychelLogger ${RAYCHELLOGGER_HEADERS} ${RAYCHELLOGGER_SOURCES})
target_include_directories(RaychelLogger PUBLIC ${RaychelLogger_SOURCE_DIR}/include)



#COMPILER FLAGS
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

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED TRUE)



#INSTALLING
include(GNUInstallDirs)
install(TARGETS RaychelLogger DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
install(FILES ${RAYCHELLOGGER_HEADERS} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/RaychelLogger)