// SPDX-License-Identifier: MIT
// TV Timing Tower - demo mode: simulated sessions that exercise every state of the tower, so the design can be
// checked in the game (any session, Time Trial included) and in the HTML preview without driving.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "model.hpp"

namespace tvt::demo {

struct Driver {
    const char *name, *code, *badge;
};

// 2026 grid (season pack). Badge keys as the game names its 2026 teams (in-game logs: Racing Bulls is
// "alphatauri_26", Audi "alfa_romeo_26", Cadillac "gm_cadillac_26").
inline const Driver kGrid[22] = {
    {"RUSSELL", "RUS", "mercedes_26"},     {"VERSTAPPEN", "VER", "red_bull_26"},
    {"LECLERC", "LEC", "ferrari_26"},      {"NORRIS", "NOR", "mclaren_26"},
    {"HAMILTON", "HAM", "ferrari_26"},     {"PIASTRI", "PIA", "mclaren_26"},
    {"ANTONELLI", "ANT", "mercedes_26"},   {"HADJAR", "HAD", "red_bull_26"},
    {"ALONSO", "ALO", "aston_martin_26"},  {"ALBON", "ALB", "williams_26"},
    {"SAINZ", "SAI", "williams_26"},       {"GASLY", "GAS", "alpine_26"},
    {"LAWSON", "LAW", "alphatauri_26"},  {"STROLL", "STR", "aston_martin_26"},
    {"HULKENBERG", "HUL", "alfa_romeo_26"},      {"OCON", "OCO", "haas_26"},
    {"BEARMAN", "BEA", "haas_26"},         {"LINDBLAD", "LIN", "alphatauri_26"},
    {"BORTOLETO", "BOR", "alfa_romeo_26"},       {"COLAPINTO", "COL", "alpine_26"},
    {"PEREZ", "PER", "gm_cadillac_26"},       {"BOTTAS", "BOT", "gm_cadillac_26"},
};

enum Scenario : int {
    race = 0, quali_q1, quali_q2, safety_car, vsc, red_flag, finish, practice, sprint_quali, gained, grid, tyres,
    banners, count
};

inline const char *scenario_name(int s)
{
    static const char *n[] = {"race", "quali_q1", "quali_q2", "safety_car", "vsc", "red_flag", "finish",
                              "practice", "sprint_quali", "gained", "grid", "tyres", "banners"};
    return s >= 0 && s < count ? n[s] : "";
}

constexpr double kScenarioSeconds = 14.0;
constexpr int kPlayer = 9; // the "player" car in the demo (ALBON, as in the reference capture)

inline double wave(double t, double phase) { return std::sin(t * 0.9 + phase); }

// One snapshot of scenario `sc` at `t` seconds into it.
inline Snapshot make(int sc, double t)
{
    Snapshot snap;
    SessionIn &s = snap.s;
    s.demo = true;
    s.racers = 22;
    s.track_m = 4381.f;
    std::vector<int> order(22);
    for (int i = 0; i < 22; i++)
        order[size_t(i)] = i;

    auto add_car = [&](int rank, int vid) -> CarIn & {
        CarIn c;
        c.vid = uint32_t(vid);
        c.pos = rank + 1;
        c.name = kGrid[vid].name;
        c.code = kGrid[vid].code;
        c.badge = kGrid[vid].badge;
        c.me = vid == kPlayer;
        snap.cars.push_back(c);
        return snap.cars.back();
    };

    const bool is_race = sc == race || sc == safety_car || sc == vsc || sc == red_flag || sc == finish || sc == gained ||
                         sc == grid || sc == tyres || sc == banners;
    if (is_race) {
        s.type = 15;
        s.laps_total = 57;
        int lead_lap = sc == finish ? 57 : sc == gained ? 2 : sc == grid ? 1 : sc == tyres ? (t < 1.0 ? 27 : 28)
                                                                                         : 22 + int(t / 7.0);
        if (sc == gained) {
            // lap 2: a start with some moves; the grid is the demo's reference order shuffled
            static const int start[22] = {0, 3, 1, 2, 5, 4, 6, 7, 9, 8, 12, 10, 11, 14, 13, 15, 17, 16, 19, 18, 21, 20};
            for (int i = 0; i < 22; i++)
                order[size_t(i)] = start[i];
        }
        // banners: P2 takes the lead (NEW RACE LEADER), then a new fastest lap
        if (sc == banners && t > 0.5)
            std::swap(order[0], order[1]);
        // a pass for P6 halfway through
        if (sc == race && t > 6.0)
            std::swap(order[5], order[6]);
        if (sc == race && t > 10.0)
            std::swap(order[11], order[12]);
        int gap_leader = 0;
        for (int r = 0; r < 22; r++) {
            int vid = order[size_t(r)];
            CarIn &c = add_car(r, vid);
            int interval = r == 0 ? 0 : 400 + (vid * 373) % 2600 + int(250 * wave(t, vid));
            if (sc == safety_car || sc == red_flag)
                interval = r == 0 ? 0 : 280 + (vid * 97) % 500;
            gap_leader += interval;
            c.gap_ahead_ms = r == 0 ? -1 : interval;
            c.gap_leader_ms = r == 0 ? -1 : gap_leader;
            c.lap = lead_lap - 1;
            c.total_m = float(lead_lap - 1) * s.track_m + 3000.f - float(gap_leader) * 0.06f;
            c.have_total = true;
            c.tyre = "MHHMSMHHHMMHMHMHHMMHSM"[vid];
            c.tyre_age = 3 + (vid * 7) % 19;
            c.speed_kmh = sc == grid ? 0.f : 250.f;
            if (sc == grid) { // on the grid: no gaps yet
                c.gap_ahead_ms = c.gap_leader_ms = -1;
                continue;
            }
            if (sc == gained) {
                // grid: the reference order (vid = grid slot - 1), so a car's gain is its vid - rank
                c.grid = vid + 1;
                continue;
            }
            if (r >= 19) { // backmarkers a lap down
                c.lap -= 1;
                c.total_m -= s.track_m;
            }
        }
        // states
        CarIn *pitter = &snap.cars[13];
        if (sc == banners && t > 3.5) { // a first fastest lap while NEW RACE LEADER shows (it waits), then beaten
            s.fastest_vid = int(snap.cars[4].vid);
            s.fastest_ms = 78450;
            if (t > 7.5) { // beaten: the banner
                s.fastest_vid = int(snap.cars[2].vid);
                s.fastest_ms = 77938;
            }
        }
        if (sc == race) {
            pitter->in_pit = t > 3.0 && t < 9.0;
            snap.cars[8].pen_s = 5.f;
            snap.cars[17].drive_through = t > 8.0;
            if (t > 4.0) {
                s.fastest_vid = int(snap.cars[3].vid);
                s.fastest_ms = 78450 - (t > 9.0 ? 600 : 0);
                if (t > 9.0)
                    s.fastest_vid = int(snap.cars[1].vid);
            }
            snap.cars[21].status = t > 11.0 ? 4 : 2; // a retirement near the end
            s.yellow = t > 11.0 ? 0x2 : 0;
        } else if (sc == safety_car) {
            // SC, "SAFETY CAR IN THIS LAP" the lap before it comes in, then the model shows SC ENDING and
            // TRACK CLEAR by itself, the same shape as VSC below
            s.sc = t < 5.0 ? 1 : 0;
            s.sc_in = t > 3.0 && t < 5.0;
            for (int i = 0; i < 4; i++)
                snap.cars[size_t(4 + i * 3)].in_pit = t > 1.0 && t < 4.0;
            s.fastest_vid = int(snap.cars[2].vid);
            s.fastest_ms = 78120;
        } else if (sc == vsc) {
            s.sc = t < 5.0 ? 2 : 0; // VSC, then the model shows VSC ENDING and TRACK CLEAR by itself
            s.fastest_vid = int(snap.cars[2].vid);
            s.fastest_ms = 78120;
        } else if (sc == red_flag) {
            s.red = true;
            for (CarIn &c : snap.cars)
                c.in_pit = true, c.gap_ahead_ms = -1, c.gap_leader_ms = -1;
            snap.cars[20].status = 7;
        } else if (sc == finish) {
            // the leader has taken the flag; cars behind finish one by one
            int done = 1 + int(t / 0.8);
            for (int r = 0; r < 22; r++) {
                CarIn &c = snap.cars[size_t(r)];
                if (r < done && r < 19) {
                    c.status = 3;
                    c.lap = 57;
                }
            }
            snap.cars[21].status = 4;
            snap.cars[20].status = 5;
            s.chequered = true;
            s.fastest_vid = int(snap.cars[1].vid);
            s.fastest_ms = 77812;
        }
        return snap;
    }

    // timed sessions
    int field = 22;
    s.clock_s = 0;
    switch (sc) {
    case quali_q1: s.type = 5; s.clock_s = 18 * 60 - 312 - t; break;
    case quali_q2: s.type = 6; s.clock_s = 15 * 60 - 721 - t; break;
    case practice: s.type = 1; s.clock_s = 60 * 60 - 1564 - t; break;
    case sprint_quali: s.type = 11; s.clock_s = 10 * 60 - 377 - t; break;
    default: s.type = 5; break;
    }
    // best laps: a spread of about 1.5 s, the player improving mid-scenario
    std::vector<std::pair<int, int>> times; // (ms, vid)
    for (int vid = 0; vid < field; vid++) {
        int ms = 78900 + vid * 71 + (vid * 131) % 97;
        if (sc == practice)
            ms += 900;
        if (vid == kPlayer && t > 5.0)
            ms -= 430;                 // personal best that moves the player up
        if (vid == 4 && t > 9.0)
            ms = 78650;                // session best: purple flash
        if ((sc == quali_q1 || sc == practice) && vid == 21)
            ms = -1;                   // no time yet
        times.push_back({ms < 0 ? 1 << 30 : ms, vid});
    }
    std::sort(times.begin(), times.end());
    for (int r = 0; r < field; r++) {
        int vid = times[size_t(r)].second;
        CarIn &c = add_car(r, vid);
        c.best_ms = times[size_t(r)].first == 1 << 30 ? -1 : times[size_t(r)].first;
        c.tyre = (sc == practice) ? "MSH"[vid % 3] : 'S';
        c.in_pit = (vid % 5 == 1) && sc != quali_q2;
        if (sc == practice && (vid == 7 || vid == 2))
            c.in_pit = true;
    }
    if (sc == quali_q2 || sc == sprint_quali) {
        // the six knocked out in Q1 stay classified at the bottom with their Q1 times
        for (int r = 16; r < 22; r++)
            snap.cars[size_t(r)].in_pit = true;
    }
    return snap;
}

} // namespace tvt::demo
