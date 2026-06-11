#include "CLobby.h"

#include <imgui.h>
#include <Windows.h>
#include <Xinput.h>

#include <string>
#include <cstdio>
#include <cwchar>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

#include <nlohmann/json.hpp>

#include "input/Input.h"
#include "global/Global.h"
#include "mcc/mcc.h"
#include "mcc/CGameManager.h"
#include "mcc/CGameGlobal.h"
#include "mcc/CGameEngine.h"
#include "render/imgui/game/xbox/CXboxColorMapping.h"
#include "render/imgui/game/xbox/CXboxMenuState.h"

namespace AlphaRing::Lobby {

// ---------------------------------------------------------------- data

static const char* kColorNames[CXboxColorMapping::kColorCount] = {
    "Steel", "Silver", "White", "Brown", "Tan", "Khaki", "Sage", "Olive",
    "Drab", "Forest", "Green", "Sea Foam", "Teal", "Aqua", "Cyan", "Blue",
    "Cobalt", "Ice", "Violet", "Orchid", "Lavender", "Maroon", "Brick",
    "Rose", "Rust", "Coral", "Peach", "Gold", "Yellow", "Pale",
};

// Approximate swatches for the picker, indexed like kColorNames.
static const ImU32 kColorSwatch[CXboxColorMapping::kColorCount] = {
    IM_COL32( 95, 100, 105, 255), IM_COL32(170, 175, 180, 255),
    IM_COL32(235, 235, 235, 255), IM_COL32(105,  70,  45, 255),
    IM_COL32(180, 160, 120, 255), IM_COL32(150, 140, 100, 255),
    IM_COL32(130, 150, 115, 255), IM_COL32(100, 110,  60, 255),
    IM_COL32(115, 105,  75, 255), IM_COL32( 35,  80,  45, 255),
    IM_COL32( 50, 140,  60, 255), IM_COL32(140, 200, 170, 255),
    IM_COL32( 30, 125, 125, 255), IM_COL32( 80, 185, 195, 255),
    IM_COL32( 60, 200, 230, 255), IM_COL32( 40,  80, 200, 255),
    IM_COL32( 35,  55, 145, 255), IM_COL32(165, 200, 230, 255),
    IM_COL32(120,  60, 180, 255), IM_COL32(190, 110, 200, 255),
    IM_COL32(190, 165, 220, 255), IM_COL32(110,  30,  45, 255),
    IM_COL32(160,  65,  50, 255), IM_COL32(225, 120, 150, 255),
    IM_COL32(175,  85,  35, 255), IM_COL32(245, 120,  95, 255),
    IM_COL32(245, 175, 130, 255), IM_COL32(210, 165,  45, 255),
    IM_COL32(235, 220,  60, 255), IM_COL32(230, 225, 180, 255),
};

static const char* kTeamNames[8] = {
    "Red", "Blue", "Green", "Orange", "Purple", "Gold", "Brown", "Pink",
};

static const ImU32 kTeamSwatch[8] = {
    IM_COL32(200, 50, 45, 255),  IM_COL32(50, 90, 200, 255),
    IM_COL32(60, 160, 60, 255),  IM_COL32(230, 140, 40, 255),
    IM_COL32(140, 60, 190, 255), IM_COL32(210, 170, 50, 255),
    IM_COL32(120, 80, 50, 255),  IM_COL32(230, 130, 180, 255),
};

static const char* kRandomNames[] = {
    "Master Chief", "Cortana", "Sgt Johnson", "The Arbiter", "Noble Six",
    "Buck", "Romeo", "Dutch", "Mickey", "Rookie", "Veronica Dare",
    "Carter-A259", "Kat-B320", "Jorge-052", "Emile-A239", "Jun-A266",
    "Dr Halsey", "Captain Keyes", "Miranda Keyes", "Lord Hood",
    "Cmdr Palmer", "Capt Lasky", "Spartan Locke", "Linda-058",
    "Kelly-087", "Fred-104", "Sam-034", "Chips Dubbo", "Foe Hammer",
    "Guilty Spark", "Tartarus", "Gravemind", "Prophet of Regret",
    "Grunt Birthday", "Hunter Bro", "Elite Major", "Jackal Sniper",
    "Brute Chieftain", "Flood Bait", "Oracle", "Reclaimer", "Demon",
    "Sierra 117", "Blue Team", "Fireteam Osiris", "Cairo Station",
};

static const char kOskChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -";
static constexpr int kOskCols = 8;
static constexpr int kOskCount = (int)(sizeof(kOskChars) - 1);
static constexpr int kOskRows = (kOskCount + kOskCols - 1) / kOskCols;
static constexpr int kNameMax = 16;

static const char* kRosterPath = "./MCC/Binaries/Win64/alpha_ring_roster.json";
static const char* kStatePath = "./MCC/Binaries/Win64/alpha_ring_menu.bin";

// ---------------------------------------------------------------- state

struct RosterEntry {
    std::string name;
    int colors[3] = {0, 0, 0};
    int team = 0;
};

enum class Stage { Hidden, QuickMenu, Countdown, Bind, Source, RosterPick, Name, Confirm };

struct PlayerSetup {
    int pad = -1;
    std::string name;
    int colors[3] = {0, 0, 0};
    int team = 0;
    bool fromRoster = false;
};

static std::vector<RosterEntry> g_roster;
static Stage g_stage = Stage::Hidden;
static PlayerSetup g_setup[4];
static int g_player = 0;        // player currently being configured
static int g_bound = 0;         // players fully confirmed
static double g_countdownEnd = 0.0;
static int g_cursor = 0;        // generic row cursor for current stage
static int g_oskX = 0, g_oskY = 0;
static int g_gridFor = -1;      // color slot being picked in the grid, -1 = closed
static int g_gridCursor = 0;
static std::string g_nameBuf;
static wchar_t g_p1Name[64] = {};
static bool g_p1HasName = false;
static MenuState g_savedState{};
static bool g_editMode = false;
static std::string g_editSourceName;  // roster entry being renamed, empty = not editing
static bool g_haveSavedSession = false;

// edge detection per pad + keyboard
static WORD g_prevButtons[4] = {};
static bool g_prevKey[256] = {};

// ---------------------------------------------------------------- helpers

static double NowSeconds() {
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
}

struct PadRead {
    bool connected = false;
    WORD held = 0;
    WORD pressed = 0;   // rising edge
    bool anyPress = false;
};

static PadRead ReadPad(int i) {
    PadRead r;
    XINPUT_STATE st;
    if (!AlphaRing::Input::GetXInputGetState(i, &st)) {
        g_prevButtons[i] = 0;
        return r;
    }
    r.connected = true;
    r.held = st.Gamepad.wButtons;
    r.pressed = r.held & ~g_prevButtons[i];
    g_prevButtons[i] = r.held;
    r.anyPress = r.pressed != 0 ||
                 st.Gamepad.bLeftTrigger > 30 || st.Gamepad.bRightTrigger > 30;
    return r;
}

static bool KeyPressed(int vk) {
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool edge = down && !g_prevKey[vk & 0xFF];
    g_prevKey[vk & 0xFF] = down;
    return edge;
}

static bool PadIsBound(int pad) {
    for (int i = 0; i < g_player; ++i)
        if (g_setup[i].pad == pad) return true;
    return false;
}

static void RandomizeName() {
    const int n = (int)(sizeof(kRandomNames) / sizeof(kRandomNames[0]));
    const char* pick = kRandomNames[rand() % n];
    // avoid handing two players the same random name in one lobby
    for (int tries = 0; tries < 8; ++tries) {
        bool taken = false;
        for (int i = 0; i < g_player; ++i)
            if (g_setup[i].name == pick) taken = true;
        if (!taken) break;
        pick = kRandomNames[rand() % n];
    }
    g_nameBuf = pick;
    if (g_nameBuf.size() > kNameMax) g_nameBuf.resize(kNameMax);
}

// ---------------------------------------------------------------- roster io

static void LoadRoster() {
    g_roster.clear();
    std::ifstream ifs(kRosterPath);
    if (!ifs) return;
    try {
        nlohmann::json j;
        ifs >> j;
        for (auto& e : j.value("players", nlohmann::json::array())) {
            RosterEntry r;
            r.name = e.value("name", "");
            if (r.name.empty()) continue;
            auto c = e.value("colors", std::vector<int>{0, 0, 0});
            for (int i = 0; i < 3 && i < (int)c.size(); ++i)
                r.colors[i] = c[i] >= 0 && c[i] < CXboxColorMapping::kColorCount ? c[i] : 0;
            r.team = e.value("team", 0) & 7;
            g_roster.push_back(r);
        }
    } catch (...) {
        g_roster.clear();
    }
}

static void SaveRoster(int sessionCount) {
    try {
        nlohmann::json players = nlohmann::json::array();
        for (auto& r : g_roster) {
            players.push_back({
                {"name", r.name},
                {"colors", {r.colors[0], r.colors[1], r.colors[2]}},
                {"team", r.team},
            });
        }
        nlohmann::json session = nlohmann::json::array();
        for (int i = 0; i < sessionCount; ++i)
            session.push_back(g_setup[i].name);
        nlohmann::json j = {{"players", players}, {"last_session", session}};
        std::ofstream ofs(kRosterPath);
        ofs << j.dump(2);
    } catch (...) {
    }
}

static void UpsertRoster(const PlayerSetup& s) {
    for (auto& r : g_roster) {
        if (r.name == s.name) {
            for (int i = 0; i < 3; ++i) r.colors[i] = s.colors[i];
            r.team = s.team;
            return;
        }
    }
    RosterEntry r;
    r.name = s.name;
    for (int i = 0; i < 3; ++i) r.colors[i] = s.colors[i];
    r.team = s.team;
    g_roster.push_back(r);
}

// ---------------------------------------------------------------- apply

static void SetProfileName(int index, const std::string& name) {
    auto profile = CGameManager::get_profile(index);
    if (!profile) return;
    int i = 0;
    for (; i < (int)name.size() && i < 1023; ++i)
        profile->name[i] = (wchar_t)name[i];
    profile->name[i] = 0;
    if (index == 0) {
        wcsncpy(g_p1Name, profile->name, 63);
        g_p1Name[63] = 0;
        g_p1HasName = true;
    }
}

static void ApplySession(int count) {
    // copy player 1's real profile into all slots as the baseline
    CGameManager::apply_profiles();

    auto* engine = GameEngine();
    auto* gg = GameGlobal();
    int game = gg ? (int)gg->current_game : (int)CGameGlobal::Halo3;

    MenuState ms{};
    ms.enabled = true;
    ms.playerCount = count;
    ms.useKM = false;
    for (int i = 0; i < 4; ++i) {
        const PlayerSetup& s = g_setup[i < count ? i : 0];
        ms.controllerIndex[i] = i < count ? s.pad : i;
        ms.teamIndex[i] = i < count ? s.team : 0;
        for (int c = 0; c < 3; ++c)
            ms.playerColors[i].colors[c] = s.colors[c];
    }

    for (int i = 0; i < count; ++i) {
        auto profile = CGameManager::get_profile(i);
        if (!profile) continue;

        profile->controller_index = g_setup[i].pad;
        SetProfileName(i, g_setup[i].name);

        int primary = CXboxColorMapping::GetColorIndex(g_setup[i].colors[0], game);
        int secondary = CXboxColorMapping::GetColorIndex(g_setup[i].colors[1], game);
        int tertiary = CXboxColorMapping::GetColorIndex(g_setup[i].colors[2], game);
        profile->profile.PlayerModelPrimaryColorIndex = primary;
        profile->profile.PlayerModelSecondaryColorIndex = secondary;
        profile->profile.PlayerModelTertiaryColorIndex = tertiary;
        profile->profile.PlayerModelPrimaryColor = primary;
        profile->profile.PlayerModelSecondaryColor = secondary;
        profile->profile.PlayerModelTertiaryColor = tertiary;

        auto xuid = CGameManager::get_xuid(i);
        if (xuid && engine)
            engine->change_team(xuid, g_setup[i].team);
    }

    auto p_setting = AlphaRing::Global::MCC::Splitscreen();
    if (p_setting) {
        p_setting->b_override = count > 1;
        p_setting->player_count = count;
        p_setting->b_player0_use_km = false;
        p_setting->b_use_player0_profile = count <= 1;
    }

    if (engine)
        engine->load_setting();

    saveMenuStateBin(ms, kStatePath);
    SaveRoster(count);
}

// ---------------------------------------------------------------- flow

bool Initialize() {
    srand((unsigned)time(nullptr));
    LoadRoster();
    return true;
}

bool IsOpen() { return g_stage != Stage::Hidden; }

const wchar_t* P1NameOverride() { return g_p1HasName ? g_p1Name : nullptr; }

void RestoreLastSession() {
    LoadRoster();
    std::ifstream ifs(kRosterPath);
    if (!ifs) return;
    try {
        nlohmann::json j;
        ifs >> j;
        auto session = j.value("last_session", std::vector<std::string>{});
        for (int i = 0; i < (int)session.size() && i < 4; ++i)
            SetProfileName(i, session[i]);
    } catch (...) {
    }
}

static void StartNewSetup() {
    for (auto& s : g_setup) s = PlayerSetup{};
    g_player = 0;
    g_bound = 0;
    g_cursor = 0;
    g_stage = Stage::Countdown;
    g_countdownEnd = NowSeconds() + 3.0;
}

void Open() {
    LoadRoster();
    g_savedState = MenuState{};
    g_haveSavedSession = loadMenuStateBin(g_savedState, kStatePath) && g_savedState.playerCount > 1;
    AlphaRing::Global::Global()->lobby_open = true;
    if (g_haveSavedSession) {
        g_cursor = g_savedState.enabled ? 1 : 0;
        for (int i = 0; i < 4; ++i) ReadPad(i);
        g_stage = Stage::QuickMenu;
    } else {
        StartNewSetup();
    }
}

void Cancel() {
    g_stage = Stage::Hidden;
    AlphaRing::Global::Global()->lobby_open = false;
}

static void FinishLobby() {
    ApplySession(g_bound);
    g_stage = Stage::Hidden;
    AlphaRing::Global::Global()->lobby_open = false;
}

static void EnterSourceStage() {
    g_cursor = g_roster.empty() ? 1 : 0;
    g_editMode = false;
    g_editSourceName.clear();
    g_stage = Stage::Source;
}

static void EnterNameStage() {
    g_nameBuf.clear();
    g_oskX = g_oskY = 0;
    g_stage = Stage::Name;
}

static void ResetConfirmUi() { g_cursor = 0; g_gridFor = -1; }

static void ConfirmPlayer() {
    if (!g_editSourceName.empty() && g_editSourceName != g_setup[g_player].name) {
        for (auto it = g_roster.begin(); it != g_roster.end(); ++it) {
            if (it->name == g_editSourceName) { g_roster.erase(it); break; }
        }
    }
    g_editSourceName.clear();
    UpsertRoster(g_setup[g_player]);
    g_bound = g_player + 1;
    if (g_bound >= 4) {
        FinishLobby();
        return;
    }
    g_player = g_bound;
    g_cursor = 0;
    g_stage = Stage::Bind;
}

// ---------------------------------------------------------------- drawing

static ImVec2 ScreenSize() { return ImGui::GetMainViewport()->Size; }

static void DrawDim() {
    auto* dl = ImGui::GetForegroundDrawList();
    ImVec2 sz = ScreenSize();
    dl->AddRectFilled({0, 0}, sz, IM_COL32(8, 12, 16, 225));
}

static void DrawCenteredText(float y, float scale, ImU32 col, const char* text) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    float size = ImGui::GetFontSize() * scale;
    ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    dl->AddText(font, size, {(ScreenSize().x - ts.x) * 0.5f, y}, col, text);
}

static const ImU32 kAccent = IM_COL32(120, 200, 80, 255);    // halo green
static const ImU32 kTextMain = IM_COL32(230, 235, 240, 255);
static const ImU32 kTextDim = IM_COL32(140, 150, 160, 255);
static const ImU32 kRowSel = IM_COL32(50, 90, 50, 160);

static void DrawHeader(const char* title) {
    ImVec2 sz = ScreenSize();
    DrawCenteredText(sz.y * 0.12f, 2.4f, kAccent, title);
    if (g_player >= 0 && g_stage != Stage::Countdown) {
        char buf[64];
        snprintf(buf, sizeof(buf), "PLAYER %d OF UP TO 4", g_player + 1);
        DrawCenteredText(sz.y * 0.12f + ImGui::GetFontSize() * 2.8f, 1.0f, kTextDim, buf);
    }
}

static void DrawFooter(const char* hints) {
    DrawCenteredText(ScreenSize().y * 0.88f, 1.0f, kTextDim, hints);
}

// Saber remaster renderer only has 2-view layouts; 3+ screens need the
// classic engine. Shown once a third player enters the flow.
static void DrawClassicGraphicsWarning() {
    if (g_player < 2) return;
    DrawCenteredText(ScreenSize().y * 0.81f, 1.1f, IM_COL32(240, 200, 70, 255),
                     "3+ PLAYERS: USE CLASSIC GRAPHICS (TAB IN-GAME) — ENHANCED ONLY SHOWS 2 SCREENS");
}

// Rows centered vertically starting at 35% height.
static float RowY(int row) {
    return ScreenSize().y * 0.35f + row * ImGui::GetFontSize() * 2.0f;
}

static void DrawRow(int row, bool selected, ImU32 col, const char* text) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImVec2 sz = ScreenSize();
    float y = RowY(row);
    float h = ImGui::GetFontSize() * 1.8f;
    if (selected)
        dl->AddRectFilled({sz.x * 0.28f, y - h * 0.15f}, {sz.x * 0.72f, y + h * 0.75f}, kRowSel, 6.0f);
    DrawCenteredText(y, 1.4f, col, text);
}

static void DrawSwatchRow(int row, bool selected, const char* label, ImU32 swatch, const char* value) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImVec2 sz = ScreenSize();
    float y = RowY(row);
    float h = ImGui::GetFontSize() * 1.8f;
    if (selected)
        dl->AddRectFilled({sz.x * 0.24f, y - h * 0.15f}, {sz.x * 0.76f, y + h * 0.75f}, kRowSel, 6.0f);

    ImFont* font = ImGui::GetFont();
    float fs = ImGui::GetFontSize() * 1.4f;
    dl->AddText(font, fs, {sz.x * 0.27f, y}, selected ? kTextMain : kTextDim, label);

    char buf[64];
    snprintf(buf, sizeof(buf), "%s %s %s", selected ? "<" : " ", value, selected ? ">" : " ");
    ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, buf);
    float vx = sz.x * 0.73f - ts.x;
    dl->AddText(font, fs, {vx, y}, kTextMain, buf);

    float box = fs * 0.8f;
    dl->AddRectFilled({vx - box - 12.0f, y + fs * 0.1f}, {vx - 12.0f, y + fs * 0.1f + box}, swatch, 3.0f);
    dl->AddRect({vx - box - 12.0f, y + fs * 0.1f}, {vx - 12.0f, y + fs * 0.1f + box}, IM_COL32(0, 0, 0, 180), 3.0f);
}

// ---------------------------------------------------------------- stages

static void StageQuickMenu() {
    DrawHeader("SPLITSCREEN");

    char resume[96];
    snprintf(resume, sizeof(resume), "SPLITSCREEN ON  (LAST SETUP: %d PLAYERS)", g_savedState.playerCount);
    bool on = g_savedState.enabled;

    DrawRow(0, g_cursor == 0, g_cursor == 0 ? kTextMain : kTextDim, resume);
    DrawRow(1, g_cursor == 1, g_cursor == 1 ? kTextMain : kTextDim, "SINGLE PLAYER");
    DrawRow(2, g_cursor == 2, g_cursor == 2 ? kTextMain : kTextDim, "NEW LOBBY SETUP");
    DrawCenteredText(RowY(4), 1.0f, kAccent, on ? "CURRENT: SPLITSCREEN ON" : "CURRENT: SINGLE PLAYER");

    DrawFooter("DPAD = MOVE      A = SELECT      B = CLOSE");

    // aggregate input from every connected pad: nothing is bound yet
    WORD pressed = 0;
    for (int i = 0; i < 4; ++i)
        pressed |= ReadPad(i).pressed;

    if ((pressed & XINPUT_GAMEPAD_DPAD_UP) && g_cursor > 0) g_cursor--;
    if ((pressed & XINPUT_GAMEPAD_DPAD_DOWN) && g_cursor < 2) g_cursor++;

    if (pressed & XINPUT_GAMEPAD_A) {
        if (g_cursor == 0) {
            g_savedState.enabled = true;
            saveMenuStateBin(g_savedState, kStatePath);
            CGameManager::reapply_menu_state();
            RestoreLastSession();
            Cancel();
        } else if (g_cursor == 1) {
            g_savedState.enabled = false;
            saveMenuStateBin(g_savedState, kStatePath);
            CGameManager::reapply_menu_state();
            Cancel();
        } else {
            StartNewSetup();
        }
        return;
    }
    if ((pressed & XINPUT_GAMEPAD_B) || KeyPressed(VK_ESCAPE))
        Cancel();
}

static void StageCountdown() {
    double left = g_countdownEnd - NowSeconds();
    if (left <= 0) {
        // swallow whatever was held while the lobby was opening
        for (int i = 0; i < 4; ++i) ReadPad(i);
        g_stage = Stage::Bind;
        return;
    }
    DrawHeader("SPLITSCREEN SETUP");
    char buf[64];
    snprintf(buf, sizeof(buf), "GAMEPAD SELECTION BEGINS IN %d", (int)left + 1);
    DrawCenteredText(ScreenSize().y * 0.45f, 1.8f, kTextMain, buf);
    DrawFooter("GRAB YOUR CONTROLLERS");
}

static void StageBind() {
    DrawHeader("SPLITSCREEN SETUP");

    char buf[96];
    snprintf(buf, sizeof(buf), "PLAYER %d — PRESS ANY BUTTON", g_player + 1);
    DrawCenteredText(ScreenSize().y * 0.42f, 1.8f, kTextMain, buf);

    if (g_player > 0) {
        snprintf(buf, sizeof(buf), "%d PLAYER%s READY", g_bound, g_bound > 1 ? "S" : "");
        DrawCenteredText(ScreenSize().y * 0.52f, 1.2f, kAccent, buf);
        DrawFooter("NEW CONTROLLER: ANY BUTTON = JOIN      PLAYER 1: START = BEGIN");
    DrawClassicGraphicsWarning();
    } else {
        DrawFooter("START+BACK OR ESC = CANCEL");
    }

    for (int i = 0; i < 4; ++i) {
        PadRead r = ReadPad(i);
        if (!r.connected) continue;

        // bound player 1 pad: Start begins the game with current players
        if (g_player > 0 && i == g_setup[0].pad) {
            if (r.pressed & XINPUT_GAMEPAD_START) {
                FinishLobby();
                return;
            }
            continue;
        }
        if (PadIsBound(i)) continue;

        if (r.anyPress) {
            g_setup[g_player] = PlayerSetup{};
            g_setup[g_player].pad = i;
            EnterSourceStage();
            return;
        }
    }

    if (KeyPressed(VK_ESCAPE))
        Cancel();
}

static void StageSource() {
    DrawHeader("CHOOSE PROFILE");

    bool hasRoster = !g_roster.empty();
    ImU32 dis = IM_COL32(80, 85, 90, 255);
    DrawRow(0, g_cursor == 0, hasRoster ? (g_cursor == 0 ? kTextMain : kTextDim) : dis, "LOAD PROFILE");
    DrawRow(1, g_cursor == 1, g_cursor == 1 ? kTextMain : kTextDim, "NEW PLAYER");
    DrawRow(2, g_cursor == 2, hasRoster ? (g_cursor == 2 ? kTextMain : kTextDim) : dis, "EDIT PROFILE");
    DrawFooter("DPAD = MOVE      A = SELECT      B = BACK");
    DrawClassicGraphicsWarning();

    PadRead r = ReadPad(g_setup[g_player].pad);
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_UP) && g_cursor > 0) g_cursor--;
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_DOWN) && g_cursor < 2) g_cursor++;
    if (!hasRoster) g_cursor = 1;

    if (r.pressed & XINPUT_GAMEPAD_A) {
        if (g_cursor == 1) {
            EnterNameStage();
        } else if (hasRoster) {
            g_editMode = g_cursor == 2;
            g_cursor = 0;
            g_stage = Stage::RosterPick;
        }
    }
    if (r.pressed & XINPUT_GAMEPAD_B) {
        g_setup[g_player].pad = -1;
        g_stage = Stage::Bind;
    }
}

static void StageRosterPick() {
    DrawHeader(g_editMode ? "EDIT PROFILE" : "LOAD PROFILE");

    // simple scrolling window of up to 7 visible entries
    int n = (int)g_roster.size();
    int first = g_cursor > 3 ? g_cursor - 3 : 0;
    if (first + 7 > n) first = n > 7 ? n - 7 : 0;

    int row = 0;
    for (int i = first; i < n && row < 7; ++i, ++row) {
        bool taken = false;
        for (int p = 0; p < g_player; ++p)
            if (g_setup[p].name == g_roster[i].name) taken = true;
        char buf[96];
        snprintf(buf, sizeof(buf), "%s%s", g_roster[i].name.c_str(), taken ? "  (IN USE)" : "");
        DrawRow(row, i == g_cursor, taken ? IM_COL32(80, 85, 90, 255) : (i == g_cursor ? kTextMain : kTextDim), buf);
    }
    DrawFooter("DPAD = MOVE      A = SELECT      B = BACK");
    DrawClassicGraphicsWarning();

    PadRead r = ReadPad(g_setup[g_player].pad);
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_UP) && g_cursor > 0) g_cursor--;
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_DOWN) && g_cursor < n - 1) g_cursor++;

    if (r.pressed & XINPUT_GAMEPAD_A) {
        const RosterEntry& e = g_roster[g_cursor];
        bool taken = false;
        for (int p = 0; p < g_player; ++p)
            if (g_setup[p].name == e.name) taken = true;
        if (!taken) {
            g_setup[g_player].name = e.name;
            for (int c = 0; c < 3; ++c) g_setup[g_player].colors[c] = e.colors[c];
            g_setup[g_player].team = e.team;
            g_setup[g_player].fromRoster = !g_editMode;
            if (g_editMode) {
                g_editSourceName = e.name;
                g_nameBuf = e.name;
                g_oskX = g_oskY = 0;
                g_stage = Stage::Name;
            } else {
                ResetConfirmUi();
                g_stage = Stage::Confirm;
            }
        }
    }
    if (r.pressed & XINPUT_GAMEPAD_B)
        EnterSourceStage();
}

static void StageName() {
    DrawHeader(g_editSourceName.empty() ? "ENTER NAME" : "EDIT NAME");

    auto* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    ImVec2 sz = ScreenSize();

    // name preview
    char preview[kNameMax + 2];
    snprintf(preview, sizeof(preview), "%s_", g_nameBuf.c_str());
    DrawCenteredText(sz.y * 0.30f, 1.8f, kTextMain, preview);

    // on-screen keyboard grid
    float fs = ImGui::GetFontSize() * 1.5f;
    float cell = fs * 1.7f;
    float gridW = kOskCols * cell;
    float x0 = (sz.x - gridW) * 0.5f;
    float y0 = sz.y * 0.42f;

    for (int i = 0; i < kOskCount; ++i) {
        int cx = i % kOskCols, cy = i / kOskCols;
        float x = x0 + cx * cell, y = y0 + cy * cell;
        bool sel = cx == g_oskX && cy == g_oskY;
        if (sel)
            dl->AddRectFilled({x, y}, {x + cell - 4, y + cell - 4}, kRowSel, 4.0f);
        char ch[4] = {kOskChars[i] == ' ' ? '_' : kOskChars[i], 0};
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, ch);
        dl->AddText(font, fs, {x + (cell - 4 - ts.x) * 0.5f, y + (cell - 4 - ts.y) * 0.5f},
                    sel ? kTextMain : kTextDim, ch);
    }

    DrawFooter("A = TYPE   X = DELETE   Y = RANDOM NAME   START = DONE   B = BACK   (KEYBOARD WORKS TOO)");

    PadRead r = ReadPad(g_setup[g_player].pad);

    if (r.pressed & XINPUT_GAMEPAD_DPAD_LEFT) g_oskX = (g_oskX + kOskCols - 1) % kOskCols;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_RIGHT) g_oskX = (g_oskX + 1) % kOskCols;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_UP) g_oskY = (g_oskY + kOskRows - 1) % kOskRows;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_DOWN) g_oskY = (g_oskY + 1) % kOskRows;

    if (r.pressed & XINPUT_GAMEPAD_A) {
        int idx = g_oskY * kOskCols + g_oskX;
        if (idx < kOskCount && (int)g_nameBuf.size() < kNameMax)
            g_nameBuf += kOskChars[idx];
    }
    if ((r.pressed & XINPUT_GAMEPAD_X) && !g_nameBuf.empty())
        g_nameBuf.pop_back();
    if (r.pressed & XINPUT_GAMEPAD_Y)
        RandomizeName();
    if (r.pressed & XINPUT_GAMEPAD_B) {
        if (!g_editSourceName.empty()) {
            g_editSourceName.clear();
            g_cursor = 0;
            g_stage = Stage::RosterPick;
        } else {
            EnterSourceStage();
        }
        return;
    }

    // physical keyboard input
    for (int vk = 'A'; vk <= 'Z'; ++vk)
        if (KeyPressed(vk) && (int)g_nameBuf.size() < kNameMax) g_nameBuf += (char)vk;
    for (int vk = '0'; vk <= '9'; ++vk)
        if (KeyPressed(vk) && (int)g_nameBuf.size() < kNameMax) g_nameBuf += (char)vk;
    if (KeyPressed(VK_SPACE) && (int)g_nameBuf.size() < kNameMax) g_nameBuf += ' ';
    if (KeyPressed(VK_OEM_MINUS) && (int)g_nameBuf.size() < kNameMax) g_nameBuf += '-';
    if (KeyPressed(VK_BACK) && !g_nameBuf.empty()) g_nameBuf.pop_back();

    bool done = (r.pressed & XINPUT_GAMEPAD_START) || KeyPressed(VK_RETURN);
    if (done && !g_nameBuf.empty()) {
        // trim trailing spaces
        while (!g_nameBuf.empty() && g_nameBuf.back() == ' ') g_nameBuf.pop_back();
        if (g_nameBuf.empty()) return;
        g_setup[g_player].name = g_nameBuf;
        g_setup[g_player].fromRoster = false;
        // returning players keep their saved colors as a starting point
        // (when editing, the picked entry's colors are already loaded)
        if (g_editSourceName.empty()) {
            for (auto& e : g_roster) {
                if (e.name == g_nameBuf) {
                    for (int c = 0; c < 3; ++c) g_setup[g_player].colors[c] = e.colors[c];
                    g_setup[g_player].team = e.team;
                }
            }
        }
        ResetConfirmUi();
        g_stage = Stage::Confirm;
    }
}

static constexpr int kGridCols = 6;

static void StageColorGrid(PlayerSetup& sPlayer) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    ImVec2 sz = ScreenSize();

    const int nc = CXboxColorMapping::kColorCount;
    const int rows = (nc + kGridCols - 1) / kGridCols;

    float cell = ImGui::GetFontSize() * 3.2f;
    float gridW = kGridCols * cell;
    float gridH = rows * cell;
    float x0 = (sz.x - gridW) * 0.5f;
    float y0 = sz.y * 0.36f;

    static const char* slotNames[3] = {"PRIMARY", "SECONDARY", "DETAIL"};
    char title[64];
    snprintf(title, sizeof(title), "%s COLOR", slotNames[g_gridFor]);
    DrawCenteredText(sz.y * 0.26f, 1.6f, kTextMain, title);
    DrawCenteredText(y0 + gridH + ImGui::GetFontSize() * 1.2f, 1.4f, kAccent, kColorNames[g_gridCursor]);

    for (int i = 0; i < nc; ++i) {
        int cx = i % kGridCols, cy = i / kGridCols;
        float x = x0 + cx * cell, y = y0 + cy * cell;
        float pad = cell * 0.12f;
        dl->AddRectFilled({x + pad, y + pad}, {x + cell - pad, y + cell - pad}, kColorSwatch[i], 4.0f);
        dl->AddRect({x + pad, y + pad}, {x + cell - pad, y + cell - pad}, IM_COL32(0, 0, 0, 200), 4.0f);
        if (i == g_gridCursor)
            dl->AddRect({x + 2, y + 2}, {x + cell - 2, y + cell - 2}, kAccent, 5.0f, 0, 3.0f);
        if (i == sPlayer.colors[g_gridFor])
            dl->AddCircleFilled({x + cell - pad - 7, y + pad + 7}, 4.0f, kAccent);
    }

    DrawFooter("DPAD = MOVE      A = SELECT      B = CANCEL");

    PadRead r = ReadPad(sPlayer.pad);
    int cx = g_gridCursor % kGridCols, cy = g_gridCursor / kGridCols;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_LEFT)  cx = (cx + kGridCols - 1) % kGridCols;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_RIGHT) cx = (cx + 1) % kGridCols;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_UP)    cy = (cy + rows - 1) % rows;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_DOWN)  cy = (cy + 1) % rows;
    g_gridCursor = cy * kGridCols + cx;
    if (g_gridCursor >= nc) g_gridCursor = nc - 1;

    if (r.pressed & XINPUT_GAMEPAD_A) {
        sPlayer.colors[g_gridFor] = g_gridCursor;
        g_gridFor = -1;
    }
    if (r.pressed & XINPUT_GAMEPAD_B)
        g_gridFor = -1;
}

static void StageConfirm() {
    PlayerSetup& s = g_setup[g_player];
    DrawHeader(s.name.c_str());

    if (g_gridFor >= 0) {
        StageColorGrid(s);
        return;
    }

    DrawSwatchRow(0, g_cursor == 0, "PRIMARY", kColorSwatch[s.colors[0]], kColorNames[s.colors[0]]);
    DrawSwatchRow(1, g_cursor == 1, "SECONDARY", kColorSwatch[s.colors[1]], kColorNames[s.colors[1]]);
    DrawSwatchRow(2, g_cursor == 2, "DETAIL", kColorSwatch[s.colors[2]], kColorNames[s.colors[2]]);
    DrawSwatchRow(3, g_cursor == 3, "TEAM", kTeamSwatch[s.team], kTeamNames[s.team]);
    DrawRow(5, g_cursor == 4, g_cursor == 4 ? kAccent : kTextDim, "READY");

    DrawFooter("DPAD = MOVE / CHANGE      A = OPEN COLOR GRID / READY      B = BACK");
    DrawClassicGraphicsWarning();

    PadRead r = ReadPad(s.pad);
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_UP) && g_cursor > 0) g_cursor--;
    if ((r.pressed & XINPUT_GAMEPAD_DPAD_DOWN) && g_cursor < 4) g_cursor++;

    int dir = 0;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_LEFT) dir = -1;
    if (r.pressed & XINPUT_GAMEPAD_DPAD_RIGHT) dir = 1;
    if (dir != 0) {
        const int nc = CXboxColorMapping::kColorCount;
        if (g_cursor < 3)
            s.colors[g_cursor] = (s.colors[g_cursor] + dir + nc) % nc;
        else if (g_cursor == 3)
            s.team = (s.team + dir + 8) % 8;
    }

    if (r.pressed & XINPUT_GAMEPAD_A) {
        if (g_cursor == 4) {
            ConfirmPlayer();
            return;
        }
        if (g_cursor < 3) {
            g_gridFor = g_cursor;
            g_gridCursor = s.colors[g_cursor];
        }
    }
    if (r.pressed & XINPUT_GAMEPAD_START) {
        ConfirmPlayer();
        return;
    }
    if (r.pressed & XINPUT_GAMEPAD_B) {
        if (s.fromRoster) {
            g_cursor = 0;
            g_stage = Stage::RosterPick;
        } else {
            g_nameBuf = s.name;
            g_stage = Stage::Name;
        }
    }
}

// ---------------------------------------------------------------- render

void Render() {
    if (g_stage == Stage::Hidden) return;

    DrawDim();

    switch (g_stage) {
        case Stage::QuickMenu: StageQuickMenu(); break;
        case Stage::Countdown: StageCountdown(); break;
        case Stage::Bind: StageBind(); break;
        case Stage::Source: StageSource(); break;
        case Stage::RosterPick: StageRosterPick(); break;
        case Stage::Name: StageName(); break;
        case Stage::Confirm: StageConfirm(); break;
        default: break;
    }
}

} // namespace AlphaRing::Lobby
