#include "Log.h"

namespace AlphaRing::Log {
    static FILE* g_logFile = nullptr;
    static CRITICAL_SECTION g_logCS;
    static bool g_csInit = false;

    // Returns path to "AlphaRing.log" next to the DLL
    static const char* GetLogPath() {
        static char path[MAX_PATH] = {};
        if (path[0] == '\0') {
            HMODULE hSelf = nullptr;
            GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&GetLogPath,
                &hSelf);
            if (hSelf && GetModuleFileNameA(hSelf, path, MAX_PATH)) {
                char* last = strrchr(path, '\\');
                if (!last) last = strrchr(path, '/');
                if (last) *(last + 1) = '\0';
                strcat(path, "AlphaRing.log");
            } else {
                strcpy(path, "AlphaRing.log");
            }
        }
        return path;
    }

    static void WriteTimestamp(FILE* f) {
        time_t now = time(nullptr);
        struct tm t;
        localtime_s(&t, &now);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);
    }

    void Early(const char* fmt, ...) {
        FILE* f = fopen(GetLogPath(), "a");
        if (!f) return;

        WriteTimestamp(f);

        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);

        fprintf(f, "\n");
        fclose(f);
    }

    void Write(const char* level, const char* msg) {
        // Use open/write/close like Early() to avoid any locking issues
        FILE* f = fopen(GetLogPath(), "a");
        if (!f) return;

        WriteTimestamp(f);
        fprintf(f, "[%s] %s\n", level, msg);
        fclose(f);
    }

    bool Init() {
        Early("=== AlphaRing Log Start ===");
        return true;
    }

    bool Shutdown() {
        Early("Shutting down logger.");
        return true;
    }
}
