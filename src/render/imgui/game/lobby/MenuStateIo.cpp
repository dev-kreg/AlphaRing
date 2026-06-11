// Binary persistence for MenuState, kept format-compatible with the
// previous Xbox-menu implementation so existing alpha_ring_menu.bin
// files keep working.

#include "render/imgui/game/xbox/CXboxMenuState.h"

#include <fstream>
#include <cstdint>

bool saveMenuStateBin(const MenuState& state, const std::string& path) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    uint8_t useKM = state.useKM ? 1 : 0;

    ofs.write(reinterpret_cast<const char*>(&state.playerCount), sizeof(state.playerCount));
    ofs.write(reinterpret_cast<const char*>(&useKM), sizeof(useKM));
    ofs.write(reinterpret_cast<const char*>(state.controllerIndex), sizeof(state.controllerIndex));
    ofs.write(reinterpret_cast<const char*>(state.teamIndex), sizeof(state.teamIndex));
    ofs.write(reinterpret_cast<const char*>(state.playerColors), sizeof(state.playerColors));

    return ofs.good();
}

bool loadMenuStateBin(MenuState& state, const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    uint8_t useKM = 0;

    ifs.read(reinterpret_cast<char*>(&state.playerCount), sizeof(state.playerCount));
    ifs.read(reinterpret_cast<char*>(&useKM), sizeof(useKM));
    state.useKM = useKM != 0;
    ifs.read(reinterpret_cast<char*>(state.controllerIndex), sizeof(state.controllerIndex));
    ifs.read(reinterpret_cast<char*>(state.teamIndex), sizeof(state.teamIndex));
    ifs.read(reinterpret_cast<char*>(state.playerColors), sizeof(state.playerColors));

    return ifs.good();
}
