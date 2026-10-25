cmake_minimum_required(VERSION 3.20)
project(frame_director_capture LANGUAGES CXX)

# Frame Director VR capture plugin (GDExtension) with two backends:
#
#   Media Foundation  Windows webcams, Elgato and other standard capture cards.
#                     Always built on Windows; needs nothing extra.
#   DELTACAST         SDI/HDMI/DP cards via the VideoMaster SDK >= 6.30.
#                     Built automatically when the SDK is found.
#
# Build on Windows (from this folder):
#   cmake -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release
#
# Options:
#   -DCAPTURE_DELTACAST=ON|OFF|AUTO   (default AUTO: use the SDK if it is installed)
#   -DDELTACAST_MOCK=ON               simulated DELTACAST board, for testing without the SDK
#
# Output: ../../addons/capture/bin/ and ../../addons/capture/capture.gdextension

set(CAPTURE_DELTACAST "AUTO" CACHE STRING "Build the DELTACAST backend: ON, OFF or AUTO")
option(DELTACAST_MOCK "Build the DELTACAST backend against a simulated board" OFF)
set(GODOT_CPP_TAG "godot-4.5-stable" CACHE STRING "godot-cpp tag (the plugin also loads in newer Godot versions)")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# godot-cpp turns C++ exceptions off by default, but the VideoMaster C++ API
# reports every error as an exception, so they must stay on.
set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
# Only build the parts of godot-cpp this plugin needs.
set(GODOTCPP_BUILD_PROFILE ${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json CACHE FILEPATH "" FORCE)

include(FetchContent)
FetchContent_Declare(
    godot-cpp
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG ${GODOT_CPP_TAG}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(godot-cpp)

add_library(capture SHARED
    src/register_types.cpp
    src/common/frame_pump.cpp
)
target_include_directories(capture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(capture PRIVATE godot::cpp)

# --- Media Foundation (Windows) ---------------------------------------------
if(WIN32)
    target_sources(capture PRIVATE
        src/mediafoundation/mf_devices.cpp
        src/mediafoundation/mf_feed.cpp
    )
    target_compile_definitions(capture PRIVATE CAPTURE_WITH_MEDIA_FOUNDATION=1 WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(capture PRIVATE mfplat mf mfreadwrite mfuuid ole32)
    if(MINGW)
        # Self-contained DLL: no MinGW runtime DLLs needed next to it.
        target_link_options(capture PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    message(STATUS "Capture plugin: Media Foundation backend enabled")
endif()

# --- DELTACAST ---------------------------------------------------------------
set(CAPTURE_SUFFIX "")
set(WITH_DELTACAST OFF)
if(DELTACAST_MOCK)
    set(WITH_DELTACAST ON)
    target_include_directories(capture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mock)
    target_compile_definitions(capture PRIVATE DELTACAST_MOCK=1)
    set(CAPTURE_SUFFIX "_mock")
    message(STATUS "Capture plugin: DELTACAST backend enabled with a SIMULATED board")
elseif(NOT CAPTURE_DELTACAST STREQUAL "OFF")
    # Same as DELTACAST's videomaster-video-monitor sample: the SDK's CMake
    # package provides VideoMasterHD and declares the VideoMasterCppApi wrapper.
    if(CAPTURE_DELTACAST STREQUAL "ON")
        find_package(VideoMasterHD 6.30 REQUIRED)
    else()
        find_package(VideoMasterHD 6.30 QUIET)
    endif()
    if(VideoMasterHD_FOUND)
        FetchContent_MakeAvailable(VideoMasterCppApi)
        target_link_libraries(capture PRIVATE VideoMasterCppApi)
        set(WITH_DELTACAST ON)
        message(STATUS "Capture plugin: DELTACAST backend enabled (VideoMaster ${VideoMasterHD_VERSION})")
    else()
        message(STATUS "Capture plugin: VideoMaster SDK not found, DELTACAST backend skipped")
    endif()
endif()
if(WITH_DELTACAST)
    target_sources(capture PRIVATE
        src/deltacast/deltacast_devices.cpp
        src/deltacast/deltacast_feed.cpp
        src/deltacast/board_host.cpp
        src/deltacast/videomaster_helper.cpp
    )
    target_compile_definitions(capture PRIVATE CAPTURE_WITH_DELTACAST=1)
endif()

# --- Output ------------------------------------------------------------------
if(WIN32)
    set(CAPTURE_PLATFORM windows)
elseif(APPLE)
    set(CAPTURE_PLATFORM macos)
else()
    set(CAPTURE_PLATFORM linux)
endif()

set(CAPTURE_OUT ${CMAKE_CURRENT_SOURCE_DIR}/../../addons/capture/bin)
set_target_properties(capture PROPERTIES
    OUTPUT_NAME "capture${CAPTURE_SUFFIX}.${CAPTURE_PLATFORM}.x86_64"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CAPTURE_OUT}
    RUNTIME_OUTPUT_DIRECTORY ${CAPTURE_OUT}
)
# Visual Studio adds a Debug/Release subfolder unless told otherwise.
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(capture PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_${config} ${CAPTURE_OUT}
        RUNTIME_OUTPUT_DIRECTORY_${config} ${CAPTURE_OUT}
    )
endforeach()

# Godot only loads the plugin once this file exists, so it is written by the
# build instead of being checked in. Without a build the project runs normally
# (simulated test pattern only on Windows).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/capture.gdextension.in
               ${CAPTURE_OUT}/../capture.gdextension @ONLY)
