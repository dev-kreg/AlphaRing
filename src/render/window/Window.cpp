#include <tchar.h>
#include "Window.h"

#include "common.h"

#include "global/Global.h"
#include "input/MenuConfig.h"
#include "render/imgui/game/lobby/CLobby.h"

#include "imgui.h"

#include "../D3d11/D3d11.h"

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace AlphaRing::Render::Window {
    WNDPROC oldWndProc = nullptr;

    //todo: WM_IME_COMPOSITION Support
    static LRESULT dWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        bool lobbyOpen = AlphaRing::Lobby::IsOpen();

        // Intercept keyboard/mouse before ImGui and the game while the
        // lobby is open. The trigger key cancels it; everything else is
        // consumed here (lobby keyboard entry polls GetAsyncKeyState).
        if (lobbyOpen) {
            if (uMsg == WM_KEYDOWN) {
                if (static_cast<int>(wParam) == g_menuConfig.keyboardVKey) {
                    AlphaRing::Lobby::Cancel();
                }
                return 0; // consume all keyboard input
            }
            switch (uMsg) {
                case WM_KEYUP:
                case WM_CHAR:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEMOVE:
                case WM_MOUSEWHEEL:
                    return 0;
            }
        }

        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        // Keyboard trigger to open the lobby (only reached when closed)
        if (uMsg == WM_KEYDOWN) {
            if (static_cast<int>(wParam) == g_menuConfig.keyboardVKey) {
                AlphaRing::Lobby::Open();
                return 0;
            }
        }

        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse)
            if (AlphaRing::Global::Global()->show_imgui)
                return true;

        return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
    }

    bool Initialize() {
        oldWndProc = (WNDPROC)SetWindowLongPtr(Graphics()->hwnd, GWLP_WNDPROC, (LONG_PTR)dWndProc);

        return oldWndProc != nullptr;
    }
}
