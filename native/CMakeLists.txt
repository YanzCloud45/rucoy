cmake_minimum_required(VERSION 3.22)
project(rucoy_tile_bot LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dobby is consumed as a prebuilt Android arm64-v8a static library.
# This deliberately avoids building current Dobby source in-tree, because
# several upstream revisions currently fail on Android/Clang.
set(DOBBY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/dobby-prebuilt")
set(DOBBY_INCLUDE_DIR "${DOBBY_ROOT}/include")
set(DOBBY_LIBRARY "${DOBBY_ROOT}/lib/libdobby.a")

if(NOT EXISTS "${DOBBY_INCLUDE_DIR}/dobby.h")
    message(FATAL_ERROR "Missing ${DOBBY_INCLUDE_DIR}/dobby.h. Run the dependency fetch step first.")
endif()

if(NOT EXISTS "${DOBBY_LIBRARY}")
    message(FATAL_ERROR "Missing ${DOBBY_LIBRARY}. Run the dependency fetch step first.")
endif()

add_library(dobby_prebuilt STATIC IMPORTED GLOBAL)
set_target_properties(dobby_prebuilt PROPERTIES
    IMPORTED_LOCATION "${DOBBY_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${DOBBY_INCLUDE_DIR}"
)

add_library(rucoy_tile_bot SHARED
    src/main.cpp
)

target_include_directories(rucoy_tile_bot PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/src"
    "${DOBBY_INCLUDE_DIR}"
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
    dobby_prebuilt
    log
    dl
    m
)
