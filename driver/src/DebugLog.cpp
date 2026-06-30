#include "DebugLog.h"

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <cstring>

namespace
{
    std::atomic<bool> g_debugLoggingEnabled{false};
}

void setDebugLoggingEnabled(bool enabled)
{
    g_debugLoggingEnabled.store(enabled, std::memory_order_relaxed);
}

bool isDebugLoggingEnabled()
{
    return g_debugLoggingEnabled.load(std::memory_order_relaxed);
}

void debugLog(const char* message)
{
    if (!message)
        return;

    OutputDebugStringA(message);

    if (!g_debugLoggingEnabled.load(std::memory_order_relaxed))
        return;

    char path[MAX_PATH] = {};
    DWORD len = GetTempPathA(MAX_PATH, path);

    if (len == 0 || len >= MAX_PATH)
        return;

    strcat_s(path, "asio2wasapi-driver.log");

    HANDLE file = CreateFileA(
        path,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
        return;

    SYSTEMTIME time = {};
    GetLocalTime(&time);

    char prefix[96] = {};
    std::snprintf(
        prefix,
        sizeof(prefix),
        "%02u:%02u:%02u.%03u ",
        time.wHour,
        time.wMinute,
        time.wSecond,
        time.wMilliseconds);

    DWORD written = 0;
    WriteFile(file, prefix, static_cast<DWORD>(std::strlen(prefix)), &written, nullptr);
    WriteFile(file, message, static_cast<DWORD>(std::strlen(message)), &written, nullptr);

    CloseHandle(file);
}
