#HEADER FILES
set(RAYCHELLOGGER_INCLUDE_PATH "${RaychelLogger_SOURCE_DIR}/include")
set(RAYCHELLOGGER_HEADERS 
    ${RAYCHELLOGGER_INCLUDE_PATH}/RaychelLogger/Helper.h
    ${RAYCHELLOGGER_INCLUDE_PATH}/RaychelLogger/Logger.h
)

#SOURCE FILES
set(RAYCHELLOGGER_SOURCES
    Logger.cpp
)



#INSTALLATION DIRECTORIES
include(GNUInstallDirs)
    set(RAYCHELLOGGER_INSTALLED_INCLUDE_PATH ${CMAKE_INSTALL_FULL_INCLUDEDIR})

    set(RAYCHELLOGGER_INSTALLED_HEADER_DIR ${RAYCHELLOGGER_INSTALLED_INCLUDE_PATH}/RaychelLogger)
    set(RAYCHELLOGGER_INSTALLED_LIB_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/RaychelLogger)

#LIBRARY
add_library(RaychelLogger SHARED ${RAYCHELLOGGER_HEADERS} ${RAYCHELLOGGER_SOURCES})
target_include_directories(RaychelLogger PUBLIC
    $<BUILD_INTERFACE:${RAYCHELLOGGER_INCLUDE_PATH}> # for headers when building
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}> # for client in install mode
)



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
        -pedantic
    )
endif()

target_compile_features(RaychelLogger PUBLIC cxx_std_17)



#INSTALLING
install(TARGETS RaychelLogger
    EXPORT RaychelLogger
    DESTINATION ${RAYCHELLOGGER_INSTALLED_LIB_DIR}
)
install(FILES ${RAYCHELLOGGER_HEADERS}
    DESTINATION ${RAYCHELLOGGER_INSTALLED_HEADER_DIR}
)
install(FILES ../RaychelLoggerConfig.cmake
    DESTINATION ${RAYCHELLOGGER_INSTALLED_LIB_DIR}
)
install(EXPORT RaychelLogger
    DESTINATION ${RAYCHELLOGGER_INSTALLED_LIB_DIR}
)