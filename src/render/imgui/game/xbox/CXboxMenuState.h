#pragma once

#include <string>

struct PlayerColor {
    int colors[3];
};

struct MenuState {
    int playerCount;
    bool useKM;
    int controllerIndex[4];
    int teamIndex[4];
    PlayerColor playerColors[4];
    // splitscreen on/off without losing the saved setup; files written by
    // older builds lack this byte and load as enabled
    bool enabled = true;
};

bool saveMenuStateBin(const MenuState& state, const std::string& path);
bool loadMenuStateBin(MenuState& state, const std::string& path);
