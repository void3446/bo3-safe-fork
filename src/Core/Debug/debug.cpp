#include "debug.hpp"

#include <cstdarg>

namespace
{
    bool g_enabled = false;
    CRITICAL_SECTION g_lock{};
    bool g_lock_initialized = false;
    std::string g_log_path;

    void write_line(const char* message) noexcept
    {
        if (!g_enabled || !g_lock_initialized)
        {
            return;
        }

        EnterCriticalSection(&g_lock);

        HANDLE file = CreateFileA(
            g_log_path.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file != INVALID_HANDLE_VALUE)
        {
            SYSTEMTIME st{};
            GetLocalTime(&st);

            char prefix[64]{};
            _snprintf_s(
                prefix,
                _countof(prefix),
                _TRUNCATE,
                "[%02u:%02u:%02u.%03u] ",
                st.wHour,
                st.wMinute,
                st.wSecond,
                st.wMilliseconds);

            DWORD bytes_written = 0;
            WriteFile(file, prefix, static_cast<DWORD>(strlen(prefix)), &bytes_written, nullptr);
            WriteFile(file, message, static_cast<DWORD>(strlen(message)), &bytes_written, nullptr);
            WriteFile(file, "\r\n", 2, &bytes_written, nullptr);
            CloseHandle(file);
        }

        LeaveCriticalSection(&g_lock);
    }
}

namespace debug
{
    void initialize(HMODULE module_handle, bool enabled) noexcept
    {
        g_enabled = enabled;

        if (g_lock_initialized)
        {
            return;
        }

        InitializeCriticalSection(&g_lock);
        g_lock_initialized = true;

        char module_path[MAX_PATH]{};
        if (GetModuleFileNameA(module_handle, module_path, static_cast<DWORD>(_countof(module_path))) == 0)
        {
            g_log_path = "bo3_patch.log";
        }
        else
        {
            std::string path_value(module_path);
            const size_t separator = path_value.find_last_of("\\/");
            if (separator == std::string::npos)
            {
                g_log_path = "bo3_patch.log";
            }
            else
            {
                g_log_path = path_value.substr(0, separator + 1) + "bo3_patch.log";
            }
        }

        if (g_enabled)
        {
            HANDLE file = CreateFileA(
                g_log_path.c_str(),
                GENERIC_WRITE,
                FILE_SHARE_READ,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);

            if (file != INVALID_HANDLE_VALUE)
            {
                CloseHandle(file);
            }

            write_line("debug logging enabled");
        }
    }

    void log(const char* format, ...) noexcept
    {
        if (!g_enabled)
        {
            return;
        }

        char buffer[1024]{};

        va_list args;
        va_start(args, format);
        _vsnprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
        va_end(args);

        write_line(buffer);
    }

    bool enabled() noexcept
    {
        return g_enabled;
    }
}
