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