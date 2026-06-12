#include "Input.h"
#include "MenuConfig.h"

#include "common.h"
#include "MinHook.h"

#include "imgui.h"
#include "global/Global.h"
#include "render/imgui/game/lobby/CLobby.h"

static HMODULE hModule;
static DWORD (WINAPI* g_pXInputGetState)(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState) WIN_NOEXCEPT;
static DWORD (WINAPI* g_pXInputSetState)(_In_ DWORD dwUserIndex, _In_ XINPUT_VIBRATION* pVibration) WIN_NOEXCEPT;

static DWORD WINAPI XInputGetStateDetour(DWORD dwUserIndex, XINPUT_STATE* pState) {
    DWORD result = g_pXInputGetState(dwUserIndex, pState);
    if (result == ERROR_SUCCESS && pState && AlphaRing::Lobby::IsOpen())
        ZeroMemory(&pState->Gamepad, sizeof(pState->Gamepad));
    return result;
}

// Games often poll the undocumented XInputGetStateEx (ordinal 100) instead of
// the named export — without this hook, controller input leaks through to the
// game's menus while the lobby is open.
static DWORD (WINAPI* g_pXInputGetStateEx)(DWORD dwUserIndex, XINPUT_STATE* pState);
static DWORD WINAPI XInputGetStateExDetour(DWORD dwUserIndex, XINPUT_STATE* pState) {
    DWORD result = g_pXInputGetStateEx(dwUserIndex, pState);
    if (result == ERROR_SUCCESS && pState && AlphaRing::Lobby::IsOpen())
        ZeroMemory(&pState->Gamepad, sizeof(pState->Gamepad));
    return result;
}

namespace AlphaRing::Input {
    bool Init() {
        if ((hModule = GetModuleHandleA("XINPUT1_3.dll")) ||
            (hModule = GetModuleHandleA("XINPUT1_4.dll")) ||
            (hModule = GetModuleHandleA("XINPUT9_1_0.dll"))) {
            LOG_INFO("XInput module found: {:p}", (void*)hModule);
            g_pXInputGetState = (decltype(g_pXInputGetState))GetProcAddress(hModule, "XInputGetState");
            g_pXInputSetState = (decltype(g_pXInputSetState))GetProcAddress(hModule, "XInputSetState");
        }

        if (hModule == nullptr) {
            LOG_ERROR("Failed to find any XInput module (1_3, 1_4, 9_1_0)");
            return false;
        }

        auto xInputAddr = (LPVOID)GetProcAddress(hModule, "XInputGetState");
        MH_STATUS mhStatus = MH_Initialize();
        if (mhStatus == MH_OK || mhStatus == MH_ERROR_ALREADY_INITIALIZED) {
            if (MH_CreateHook(xInputAddr, &XInputGetStateDetour,
                              reinterpret_cast<LPVOID*>(&g_pXInputGetState)) == MH_OK) {
                MH_EnableHook(xInputAddr);
            }

            // ordinal 100 = XInputGetStateEx (not exported by name; absent in 9_1_0)
            if (auto xInputExAddr = (LPVOID)GetProcAddress(hModule, (LPCSTR)100)) {
                if (MH_CreateHook(xInputExAddr, &XInputGetStateExDetour,
                                  reinterpret_cast<LPVOID*>(&g_pXInputGetStateEx)) == MH_OK) {
                    MH_EnableHook(xInputExAddr);
                    LOG_INFO("Hooked XInputGetStateEx (ordinal 100)");
                }
            } else {
                LOG_INFO("XInputGetStateEx not present in this xinput module");
            }
        }

        g_menuConfig = MenuConfig::load();

        return true;
    }

    bool Shutdown() {
        return true;
    }

    bool GetXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
        if (!g_pXInputGetState || !pState) return false;
        memset(pState, 0, sizeof(XINPUT_STATE));
        return g_pXInputGetState(dwUserIndex, pState) == ERROR_SUCCESS;
    }

    void SetState(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration) {
        if (!g_pXInputSetState) return;
        g_pXInputSetState(dwUserIndex, pVibration);
    }

    bool Update() {
        static bool b_pressed    = false;
        static WORD prevButtons  = 0;

        XINPUT_STATE state;
        if (!GetXInputGetState(0, &state))
            return false;

        WORD buttons     = state.Gamepad.wButtons;
        WORD justPressed = buttons & ~prevButtons;
        prevButtons      = buttons;

        // configurable debug UI keyboard key
        int debugKey = g_menuConfig.debugKeyboardVKey;
        if (GetAsyncKeyState(debugKey) & 0x8000) {
            AlphaRing::Global::Global()->show_imgui = !AlphaRing::Global::Global()->show_imgui;
            return false;
        }
        
        // configurable debug UI combo
        WORD debugCombo = g_menuConfig.debugComboMask;
        if ((buttons & debugCombo) == debugCombo && (justPressed & debugCombo) != 0) {
            AlphaRing::Global::Global()->show_imgui = !AlphaRing::Global::Global()->show_imgui;
            return false;
        }

        // configurable splitscreen lobby combo
        WORD menuCombo = g_menuConfig.controllerComboMask;
        if ((buttons & menuCombo) == menuCombo && (justPressed & menuCombo) != 0) {
            if (AlphaRing::Lobby::IsOpen()) AlphaRing::Lobby::Cancel();
            else AlphaRing::Lobby::Open();
        }

        // the lobby polls every controller itself
        if (AlphaRing::Lobby::IsOpen())
            return true;


        // thumbstick mouse control for debug UI
        if (AlphaRing::Global::Global()->show_imgui) {
            const auto f_speed = [](SHORT x, SHORT y) -> ImVec2 {
                const auto speed = 5.0f;
                const auto f_normalize = [](SHORT sThumb) -> float {
                    const auto deadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                    return (abs(sThumb) > deadZone) ? (sThumb / 32767.0f) : 0.0f;
                };
                return {f_normalize(x) * speed, -f_normalize(y) * speed};
            };

            ImGuiIO& io = ImGui::GetIO();

            if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
                if (!b_pressed) {
                    io.MouseDown[0] = true;
                    b_pressed = true;
                }
            } else if (b_pressed) {
                io.MouseDown[0] = false;
                b_pressed = false;
            }

            io.MousePos += f_speed(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
        }

        return true;
    }
}
