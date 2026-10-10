cmake_minimum_required(VERSION 3.25)
project(ImpliedVolatility LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

add_executable(implied_volatility src/main.cpp)
configure_file(${CMAKE_SOURCE_DIR}/plot.py ${CMAKE_BINARY_DIR}/plot.py COPYONLY)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(implied_volatility PRIVATE -std=c++2c)
elseif(MSVC)
    target_compile_options(implied_volatility PRIVATE /std:c++latest)
else()
    message(FATAL_ERROR "A C++26-capable compiler is required.")
endif()

target_link_libraries(implied_volatility PRIVATE Threads::Threads)


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND UNIX AND NOT APPLE)
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libstdc++.so.6
        OUTPUT_VARIABLE LIBSTDCPP_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(IS_ABSOLUTE "${LIBSTDCPP_PATH}")
        get_filename_component(LIBSTDCPP_DIR "${LIBSTDCPP_PATH}" DIRECTORY)
        set_target_properties(implied_volatility PROPERTIES BUILD_RPATH "${LIBSTDCPP_DIR}")
    endif()
endif()
