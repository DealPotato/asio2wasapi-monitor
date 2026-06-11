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

# Standalone ASIO -> WASAPI monitor app
add_executable(asio2wasapi-monitor
    src/main.cpp
    src/Config.cpp
    src/DeviceList.cpp
    src/AudioBridge.cpp
)

target_link_libraries(asio2wasapi-monitor
    PRIVATE
        rtaudio
)

target_include_directories(asio2wasapi-monitor
    PRIVATE
        src
)

target_compile_definitions(asio2wasapi-monitor
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

# Copy rtaudio.dll next to our exe after every build
add_custom_command(TARGET asio2wasapi-monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:rtaudio>
        $<TARGET_FILE_DIR:asio2wasapi-monitor>
)

# Virtual ASIO driver DLL
add_library(asio2wasapi-virtual-asio SHARED
    driver/src/Asio2WasapiDriver.cpp
    driver/src/Asio2WasapiCom.cpp
    driver/src/dllmain.cpp
    driver/src/asio2wasapi-virtual-asio.def
    driver/src/DebugLog.cpp
    driver/src/WasapiOutputSink.cpp
    driver/src/AsioInputSource.cpp
    driver/src/DriverConfig.cpp
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
)

add_executable(asio2wasapi-devices
    tools/device-list/main.cpp
)

target_include_directories(asio2wasapi-devices PRIVATE
    external/rtaudio/include
)

target_link_libraries(asio2wasapi-devices PRIVATE
    rtaudio
)

set_target_properties(asio2wasapi-virtual-asio
    PROPERTIES
        OUTPUT_NAME "asio2wasapi-virtual-asio"
)