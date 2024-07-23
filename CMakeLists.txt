cmake_minimum_required(VERSION 3.10)
project(PLCLToWeb VERSION 0.0.0)

include(FetchContent)
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_STANDARD 23)
configure_file(cmake/CMakeInfo.hpp.in include/CMakeInfo.hpp)

pkg_check_modules(PLCL IMPORTED_TARGET PLCL)

if(NOT PLCL_FOUND)
    message(STATUS "libPLCL not found, fetching it with FetchContent.")
    FetchContent_Declare(PLCL
            GIT_REPOSITORY git@github.com:PapyrusLikeConfigurationLanguage/libPLCL.git
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
)

target_link_libraries(${PROJECT_NAME} PRIVATE PLCL)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
