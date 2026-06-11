#pragma once

// Couch-lobby splitscreen wizard.
//
// Start+Back opens a full-screen, controller-driven setup flow:
//   countdown -> player 1 presses any button to bind their pad
//   -> load a saved profile or create a new one (name via on-screen
//      keyboard, or randomized Halo-themed names)
//   -> confirm colors + team
//   -> next controller joins the same way, or player 1 presses Start
//      to begin with the players bound so far (max 4).
//
// Profiles persist in alpha_ring_roster.json next to the DLL; picking a
// saved name restores that player's colors and team.

namespace AlphaRing::Lobby {
    // Loads the roster from disk. Call once after ImGui is initialized.
    bool Initialize();

    void Open();
    void Cancel();
    bool IsOpen();

    // Update + draw. Call once per frame inside an active ImGui frame.
    void Render();

    // Player 1's custom display name, or nullptr when unset. Used by the
    // get_xbox_user_id hook so the in-game name matches the lobby name.
    const wchar_t* P1NameOverride();

    // Re-applies the last session's player names after the menu state has
    // been restored from disk on startup.
    void RestoreLastSession();
}
