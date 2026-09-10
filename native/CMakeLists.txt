cmake_minimum_required(VERSION 3.22)
project(rucoy_tile_bot LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dobby settings. Pinned source is cloned by GitHub Actions.
set(DOBBY_DEBUG OFF CACHE BOOL "" FORCE)
set(DOBBY_GENERATE_SHARED OFF CACHE BOOL "" FORCE)
set(NearBranch ON CACHE BOOL "" FORCE)
set(FullFloatingPointRegisterPack OFF CACHE BOOL "" FORCE)
set(Plugin.SymbolResolver OFF CACHE BOOL "" FORCE)
set(Plugin.ImportTableReplace OFF CACHE BOOL "" FORCE)
set(Plugin.Android.BionicLinkerUtil OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(DOBBY_BUILD_TEST OFF CACHE BOOL "" FORCE)

set(DOBBY_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/Dobby")
if(NOT EXISTS "${DOBBY_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Dobby source missing at ${DOBBY_DIR}. Run through the GitHub Action or clone the pinned dependency first.")
endif()

add_subdirectory("${DOBBY_DIR}" "${CMAKE_BINARY_DIR}/dobby")

add_library(rucoy_tile_bot SHARED
    src/main.cpp
)

target_include_directories(rucoy_tile_bot PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/src"
    "${DOBBY_DIR}/include"
)

target_compile_options(rucoy_tile_bot PRIVATE
    -O2
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -ffunction-sections
    -fdata-sections
)

target_link_options(rucoy_tile_bot PRIVATE
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
)

target_link_libraries(rucoy_tile_bot PRIVATE
    dobby
    log
    dl
)
