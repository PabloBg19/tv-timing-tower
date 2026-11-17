// SPDX-License-Identifier: MIT
// TV Timing Tower - the badge of a team the game draws from a rendered texture (My Team): its texture path, read from
// the game's own session cars the way its HUD does. Swofty's live::team_badge gives the key of a stock team's badge
// ("ferrari_26": #overrides.osd.timing_tower.team_badges_hud.<key>) and nothing for My Team, whose badge the game
// renders at run time. Read off the HUD's badge builder (F1_25.exe 1794312636, 0x1417617a0) and the texture name
// builder it calls (0x14bd1e150):
//
//   g_Game_Manager -> +0xc0 -> +0x88 -> +0x260 the session: its cars from +0x2e0 to +0x2e8, 0x88 bytes each, the
//     vehicle id the byte at +0x82
//   a car +0x68: its badge, a type byte (0 ePreLoaded, 1 eUserDefined, 2 eNetworkReplicated) and at +0x6a the badge
//     id (0x3fff: none, the game draws render_badge_default.tga)
//   the texture: "eaid://ui_package/art/textures/render_badge_" + "<type> <id>" ("render_badge_eUserDefined 3")
//
// Only for a car without a stock badge key; every read guarded and every value checked (the list's size and stride,
// the vehicle id, the type). Anything else: "" (the team's colour chip, as before). Nothing is ever written.
#pragma once
#include <cstdint>
#include <string>

namespace tvt::mtbadge {

constexpr uintptr_t kMgrGame = 0xc0, kGameSess = 0x88, kSession = 0x260;
constexpr uintptr_t kCars = 0x2e0, kCarsEnd = 0x2e8;
constexpr size_t kCarSize = 0x88, kMaxCars = 64;
constexpr uintptr_t kVid = 0x82, kBadgeType = 0x68, kBadgeId = 0x6a;
constexpr uint16_t kNoBadge = 0x3fff;

inline std::string texture(uint8_t type, uint16_t id)
{
    static const char *kTypes[3] = {"ePreLoaded", "eUserDefined", "eNetworkReplicated"};
    if (type > 2 || id >= kNoBadge)
        return {};
    return std::string("eaid://ui_package/art/textures/render_badge_") + kTypes[type] + " " + std::to_string(id);
}

// Read(at, dst, n): a guarded read. `mgr_at`: g_Game_Manager's address in memory. The rendered badge's texture path
// of vehicle `vid`, or "" (`why` says why when the layout didn't look right).
template <class Read> std::string find(Read &rd, uintptr_t mgr_at, uint32_t vid, std::string &why)
{
    uintptr_t p = 0;
    auto hop = [&](uintptr_t at) {
        p = 0;
        return rd(at, &p, 8) && p && !(p & 7);
    };
    if (!mgr_at || !hop(mgr_at) || !hop(p + kMgrGame) || !hop(p + kGameSess) || !hop(p + kSession))
        return why = "no session", std::string();
    uintptr_t v[2] = {};
    if (!rd(p + kCars, v, sizeof v) || !v[0] || v[1] < v[0] || (v[1] - v[0]) % kCarSize ||
        (v[1] - v[0]) / kCarSize > kMaxCars)
        return why = "the session's cars don't look right", std::string();
    // the whole list: each vehicle id once (a list that isn't the session's cars would rarely pass that)
    const size_t n = (v[1] - v[0]) / kCarSize;
    uint8_t cars[kMaxCars * kCarSize];
    if (!n || !rd(v[0], cars, n * kCarSize))
        return why = "the session's cars are unreadable", std::string();
    uint64_t seen = 0;
    const uint8_t *mine = nullptr;
    for (size_t i = 0; i < n; i++) {
        const uint8_t *car = cars + i * kCarSize;
        const uint8_t id = car[kVid];
        if (id >= 64 || (seen >> id & 1))
            return why = "the session's cars don't look right", std::string();
        seen |= uint64_t(1) << id;
        if (id == vid)
            mine = car;
    }
    if (!mine)
        return {};
    return texture(mine[kBadgeType], uint16_t(mine[kBadgeId] | (mine[kBadgeId + 1] << 8)));
}

}
