#include "module_definition.h"

#include <filesystem>
#include "log/Log.h"

ModuleDefinition::ModuleDefinition(const char *moduleName, std::initializer_list<const char*> funcs) {
    AlphaRing::Log::Early("ModuleDefinition: loading wrapper for '%s'", moduleName);

    // Get System Directory
    wchar_t systemPath[MAX_PATH];
    if (!GetSystemDirectoryW(systemPath, MAX_PATH)) {
        AlphaRing::Log::Early("ERROR: GetSystemDirectoryW failed (error %lu)", GetLastError());
        ExitProcess(1);
    }

    // Load DLL
    std::filesystem::path path(systemPath);
    path.append(moduleName);
    AlphaRing::Log::Early("ModuleDefinition: loading real DLL from '%ls'", path.c_str());
    if ((m_hModule = LoadLibraryW(path.c_str())) == nullptr) {
        AlphaRing::Log::Early("ERROR: LoadLibraryW failed for '%s' (error %lu)", moduleName, GetLastError());
        ExitProcess(1);
    }
    AlphaRing::Log::Early("ModuleDefinition: real DLL loaded at %p", (void*)m_hModule);

    // Find Function
    void *func_ptr;
    for (auto func: funcs) {
        if ((func_ptr = GetProcAddress(m_hModule, func)) == nullptr) {
            AlphaRing::Log::Early("ERROR: GetProcAddress failed for '%s' (error %lu)", func, GetLastError());
            ExitProcess(1);
        }
        AlphaRing::Log::Early("ModuleDefinition: resolved '%s' -> %p", func, func_ptr);

        m_funcs.push_back(func_ptr);
    }
    AlphaRing::Log::Early("ModuleDefinition: wrapper ready (%zu functions)", m_funcs.size());
}

void *ModuleDefinition::GetFunc(int index) {
    if (index < 0 || index >= (int)m_funcs.size()) return nullptr;
    return m_funcs[index];
}
