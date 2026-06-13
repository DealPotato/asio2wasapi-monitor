cmake_minimum_required(VERSION 3.24)

project(ASIO2WASAPIMonitor
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RtAudio backend options
set(RTAUDIO_API_WASAPI ON CACHE BOOL "" FORCE)
set(RTAUDIO_API_ASIO ON CACHE BOOL "" FORCE)

add_subdirectory(external/rtaudio)

# Virtual ASIO driver DLL
add_library(asio2wasapi-virtual-asio SHARED
    driver/src/Asio2WasapiDriver.cpp
    driver/src/Asio2WasapiCom.cpp
    driver/src/DebugLog.cpp
    driver/src/DriverConfig.cpp
    driver/src/WasapiOutputSink.cpp
    driver/src/AsioInputSource.cpp
    driver/src/dllmain.cpp
    driver/src/asio2wasapi-virtual-asio.def
)

target_include_directories(asio2wasapi-virtual-asio
    PRIVATE
        driver/include
        external/rtaudio/include
)

target_compile_definitions(asio2wasapi-virtual-asio
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(asio2wasapi-virtual-asio
    PRIVATE
        rtaudio
        ole32
        user32
        shell32
        avrt
)

set_target_properties(asio2wasapi-virtual-asio
    PROPERTIES
        OUTPUT_NAME "asio2wasapi-virtual-asio"
)

# Device scanner helper
add_executable(asio2wasapi-devices
    apps/device-list/main.cpp
)

target_include_directories(asio2wasapi-devices
    PRIVATE
        external/rtaudio/include
)

target_compile_definitions(asio2wasapi-devices
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(asio2wasapi-devices
    PRIVATE
        rtaudio
)

set_target_properties(asio2wasapi-devices
    PROPERTIES
        OUTPUT_NAME "asio2wasapi-devices"
)

# Local runtime/install paths
set(ASIO2WASAPI_INSTALL_DIR
    "${CMAKE_SOURCE_DIR}/installed-driver"
)

set(ASIO2WASAPI_CONTROL_PANEL_PUBLISH_DIR
    "${CMAKE_BINARY_DIR}/control-panel-publish"
)

# Publish C# control panel
add_custom_target(publish-control-panel
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${ASIO2WASAPI_CONTROL_PANEL_PUBLISH_DIR}"
    COMMAND dotnet publish
        "${CMAKE_SOURCE_DIR}/apps/control-panel"
        -c Release
        -r win-x64
        --self-contained true
        -p:PublishSingleFile=true
        -p:IncludeNativeLibrariesForSelfExtract=true
        -o "${ASIO2WASAPI_CONTROL_PANEL_PUBLISH_DIR}"
    COMMENT "Publishing ASIO2WASAPI Control Panel"
    VERBATIM
)

# Build everything and copy runtime files to installed-driver
add_custom_target(install-local
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASIO2WASAPI_INSTALL_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:asio2wasapi-virtual-asio>"
        "${ASIO2WASAPI_INSTALL_DIR}/asio2wasapi-virtual-asio.dll"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:rtaudio>"
        "${ASIO2WASAPI_INSTALL_DIR}/rtaudio.dll"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:asio2wasapi-devices>"
        "${ASIO2WASAPI_INSTALL_DIR}/asio2wasapi-devices.exe"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASIO2WASAPI_CONTROL_PANEL_PUBLISH_DIR}/asio2wasapi-control.exe"
        "${ASIO2WASAPI_INSTALL_DIR}/asio2wasapi-control.exe"

    DEPENDS
        asio2wasapi-virtual-asio
        asio2wasapi-devices
        publish-control-panel

    COMMENT "Installing ASIO2WASAPI runtime files to installed-driver"
    VERBATIM
)