cmake_minimum_required(VERSION 3.10)
project(PLCLToWeb VERSION 0.0.0)

include(FetchContent)
include(GNUInstallDirs)

find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 23)
configure_file(cmake/CMakeInfo.hpp.in include/CMakeInfo.hpp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(PkgConfig_FOUND)
    message(STATUS "PkgConfig found. Checking for libPLCL.")
    pkg_check_modules(PLCL IMPORTED_TARGET PLCL)
else()
    message(WARNING "PkgConfig not found.")
endif()

if(NOT PLCL_FOUND)
    message(STATUS "libPLCL not found, fetching it with FetchContent.")
    FetchContent_Declare(PLCL
            GIT_REPOSITORY https://github.com/PapyrusLikeConfigurationLanguage/libPLCL.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(PLCL)
    include_directories($PLCL_SOURCE_DIR}/include)
else()
    message(STATUS "PLCL found, using it.")
    include_directories(${PLCL_INCLUDEDIR})
endif()

include_directories("${PROJECT_BINARY_DIR}/include")

add_executable(${PROJECT_NAME} src/main.cpp
        src/HTML.cpp
        src/CSS.cpp
        src/Cli.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE PLCL)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
