// SPDX-License-Identifier: MIT
// TV Timing Tower - the tower model. Pure C++: no game access, no Windows, no allocation-heavy work per frame.
// Inputs: a Snapshot (session + cars in classification order) and a Config. Output: a View (header + rows) that
// the publisher turns into HUD leaves and the preview tool turns into JSON. Same inputs, same view (plus the
// small amount of time-based state: arrows, flashes, auto gap mode, flag transitions).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace tvt {

constexpr int kRows = 24;    // rows the HUD record composes (a career with 12 teams has 24 cars)
constexpr int kSlots = 30;   // per-car name slots per row (vehicle ids up to 29: a My Team grid, as Project Halo found)
constexpr int kTeams = 12;   // team identities (preview logos)

// ---------------------------------------------------------------------------------------------- session kinds
enum class Kind { none, practice, quali, sprint_quali, race, time_trial };

inline Kind kind_of(int type)
{
    if (type >= 1 && type <= 4)
        return Kind::practice;
    if (type >= 5 && type <= 9)
        return Kind::quali;
    if (type >= 10 && type <= 14)
        return Kind::sprint_quali;
    if (type >= 15 && type <= 17)
        return Kind::race;
    if (type == 18)
        return Kind::time_trial;
    return Kind::none;
}

inline const char *session_label(int type)
{
    static const char *names[] = {"",    "P1",  "P2",  "P3",  "PRACTICE", "Q1",  "Q2", "Q3", "QUALIFYING", "OSQ",
                                  "SQ1", "SQ2", "SQ3", "SPRINT QUALI", "OSQ", "RACE", "RACE 2", "RACE 3", "TIME TRIAL"};
    return type >= 0 && type <= 18 ? names[type] : "";
}

// Knockout segment: 1 for Q1/SQ1, 2 for Q2/SQ2, 3 for Q3/SQ3, 0 otherwise.
inline int knockout_segment(int type)
{
    switch (type) {
    case 5: case 10: return 1;
    case 6: case 11: return 2;
    case 7: case 12: return 3;
    default: return 0;
    }
}

// Cars that go through a segment with `field` racers: Q1 -> 10 + (n-10)/2 (16 of 22, 15 of 20), Q2 -> 10.
inline int knockout_cut(int segment, int field)
{
    if (field <= 10)
        return field;
    if (segment == 1)
        return 10 + (field - 10) / 2;
    if (segment == 2)
        return 10;
    return field;
}

// ------------------------------------------------------------------------------------------------- formatting
// Timed sessions: thousandths ("+0.234", "+1:02.345").
inline std::string fmt_gap_ms(int ms);

// 2026 broadcast: one decimal, truncated ("+0.9"); over a minute "+1:05.3". `thousandths`: the classic "+0.934".
inline std::string fmt_gap_race(int ms, bool thousandths = false)
{
    if (thousandths)
        return fmt_gap_ms(ms);
    char b[24];
    if (ms < 0)
        ms = 0;
    int tenths = ms / 100;
    if (tenths >= 600)
        std::snprintf(b, sizeof b, "+%d:%02d.%d", tenths / 600, (tenths / 10) % 60, tenths % 10);
    else
        std::snprintf(b, sizeof b, "+%d.%d", tenths / 10, tenths % 10);
    return b;
}

// Timed sessions: thousandths ("+0.234", "+1:02.345").
inline std::string fmt_gap_ms(int ms)
{
    char b[24];
    if (ms < 0)
        ms = 0;
    if (ms >= 60000)
        std::snprintf(b, sizeof b, "+%d:%02d.%03d", ms / 60000, (ms / 1000) % 60, ms % 1000);
    else
        std::snprintf(b, sizeof b, "+%d.%03d", ms / 1000, ms % 1000);
    return b;
}

inline std::string fmt_lap(int ms)
{
    char b[24];
    if (ms < 0)
        return "";
    std::snprintf(b, sizeof b, "%d:%02d.%03d", ms / 60000, (ms / 1000) % 60, ms % 1000);
    return b;
}

// a target time as the qualifying card shows it: "30.752" under a minute, "1:04.773" over
inline std::string fmt_lap_short(int ms)
{
    char b[24];
    if (ms < 0)
        return "";
    if (ms < 60000)
        std::snprintf(b, sizeof b, "%d.%03d", ms / 1000, ms % 1000);
    else
        std::snprintf(b, sizeof b, "%d:%02d.%03d", ms / 60000, (ms / 1000) % 60, ms % 1000);
    return b;
}

inline std::string fmt_laps_down(int n)
{
    if (n <= 0)
        return "";
    char b[16];
    std::snprintf(b, sizeof b, "+%d Lap%s", n, n == 1 ? "" : "s");
    return b;
}

inline std::string fmt_clock(double seconds)
{
    if (seconds < 0)
        seconds = 0;
    int s = int(seconds);
    char b[16];
    if (s >= 3600)
        std::snprintf(b, sizeof b, "%d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
    else
        std::snprintf(b, sizeof b, "%d:%02d", s / 60, s % 60);
    return b;
}

// --------------------------------------------------------------------------------------------- team mapping
// Team identity for the HTML preview's logos (the game draws its own badges). The game's badge keys look like "ferrari_2026"; we match by
// substring so new keys of the same team keep working.
inline const char *team_name(int i)
{
    static const char *n[kTeams] = {"mercedes", "ferrari", "redbull", "mclaren", "astonmartin", "alpine",
                                    "williams", "racingbulls", "haas", "sauber", "audi", "cadillac"};
    return i >= 0 && i < kTeams ? n[i] : "";
}

// a team's name as the broadcast's constructors' table writes it
inline const char *team_label(int i)
{
    static const char *n[kTeams] = {"MERCEDES", "FERRARI", "RED BULL", "MCLAREN", "ASTON MARTIN", "ALPINE",
                                    "WILLIAMS", "RACING BULLS", "HAAS", "SAUBER", "AUDI", "CADILLAC"};
    return i >= 0 && i < kTeams ? n[i] : "";
}

inline int team_index(const std::string &badge)
{
    std::string k;
    for (char c : badge)
        if (c != '_' && c != ' ' && c != '-')
            k += char(std::tolower(static_cast<unsigned char>(c)));
    auto has = [&](const char *s) { return k.find(s) != std::string::npos; };
    if (k.empty())
        return -1;
    // The broadcast icons are the 2026 teams'. Only the 2026 season's badges (keys ending in 26: "ferrari_26",
    // "alfa_romeo_26") get one; any other season keeps the game's own badge, which is right for it (an older
    // season's "alfa_romeo" is Sauber / Alfa Romeo, not 2026's Audi, and used to come out as Kick).
    if (k.size() < 2 || k.compare(k.size() - 2, 2, "26") != 0)
        return -1;
    if (has("mercedes"))
        return 0;
    if (has("ferrari"))
        return 1;
    if (has("racingbull") || has("visa") || has("alphatauri") || has("tororosso") || k.rfind("rb", 0) == 0)
        return 7;
    if (has("redbull"))
        return 2;
    if (has("mclaren"))
        return 3;
    if (has("aston"))
        return 4;
    if (has("alpine") || has("renault"))
        return 5;
    if (has("williams"))
        return 6;
    if (has("haas"))
        return 8;
    if (has("audi"))
        return 10;
    if (has("alfaromeo26") || has("alfaromeo2026"))
        return 10; // the game keeps Sauber's old key for Audi in 2026
    if (has("sauber") || has("kick") || has("alfa"))
        return 9;
    if (has("cadillac") || has("andretti") || has("gm"))
        return 11;
    return -1;
}

// ------------------------------------------------------------------------------------------------- inputs
struct CarIn {
    uint32_t vid = 0;
    int pos = 0;              // 1-based, 0 unknown
    int status = 2;           // 2 running, 3 finished, 4 DNF, 5 DSQ, 6 NC, 7 retired; 0 unknown
    bool in_pit = false;      // pit lane or garage
    int lap = 0;              // the game's counter, 0 on lap 1
    float total_m = 0;        // session distance (for laps down)
    bool have_total = false;
    char tyre = 0;            // 'S' 'M' 'H' 'I' 'W'
    int tyre_age = -1;        // laps on this set, -1 unknown
    float pen_s = 0;
    bool drive_through = false, stop_go = false;
    int gap_ahead_ms = -1;    // -1 invalid
    int gap_leader_ms = -1;
    int grid = 0;             // starting position when the source knows it (demo); 0: the tower records it
    int pit_stops = -1;       // the game's own count, -1 unknown
    float speed_kmh = -1;     // -1 unknown (starting tyres: the grid stands still before the start)
    int best_ms = -1;         // best valid lap
    int last_ms = -1;         // last completed lap
    bool out_lap = false;     // on track after the pit lane, the line not crossed since (timed sessions: "Out Lap")
    bool me = false;
    int best_sec[3] = {-1, -1, -1}; // the game's: the best lap's sectors (ms)
    int pb_sec[3] = {-1, -1, -1};   // the game's: the car's best of each sector
    std::string badge;        // the game's team badge key ("ferrari_2026") or ""
    std::string badge_tex;    // a rendered badge's texture path (My Team), used when there is no key
    std::string name;         // only demo mode: the driver's surname
    std::string code;         // only demo mode: three-letter code
};

// The texture of a car's badge for the game's logo unit: the stock badge by its key (the broadcast set in colour, which
// follows logo mods, or the HUD's white set), else a rendered badge (My Team), else "" (the team's colour chip)
inline std::string badge_texture(const CarIn &c, bool hud_white)
{
    if (!c.badge.empty())
        return std::string(hud_white ? "#overrides.osd.timing_tower.team_badges_hud."
                                     : "#overrides.osd.timing_tower.team_badges_broadcast.") +
               c.badge;
    return c.badge_tex;
}

struct SessionIn {
    int type = 0;
    int racers = 0;           // competitors in the session (not the safety car)
    float track_m = 0;        // track length
    int sc = 0;               // 0 none, 1 safety car, 2 virtual
    bool formation = false;   // formation lap (the grid is the order now)
    bool sc_in = false;       // the safety car is coming in (inferred)
    bool red = false;
    uint8_t yellow = 0;       // bit i: sector i+1
    bool chequered = false;
    int laps_total = -1;      // -1: take it from the game binding
    double clock_s = -1;      // -1: take it from the game binding
    int fastest_vid = -1;     // race fastest lap holder
    int fastest_ms = -1;
    bool demo = false;
    bool game_times = false;  // best_ms / last_ms are the game's own (laptimes.hpp): show them in timed sessions
};

// One driver of the career's drivers' championship (standings.hpp)
struct ChampEntry {
    uint32_t stat = 0;  // the driver's stat id
    int points = 0;     // the game's total so far
    int rank = 0;       // 1-based, the game's order
    int vid = -1;       // his car in this session, -1 none
};

// The player's lap (qcard.hpp measures it): the qualifying card's input
struct MyLap {
    bool known = false;       // the player's car is on track and its line crossings are being followed
    bool flying = false;      // a lap begun at the line with no pit lane since (not an out lap)
    int t_ms = -1;            // into that lap (the addon's clock: pauses left out)
    int split[2] = {-1, -1};  // to the end of sector 1 and of sector 2, ms (the game's when it has them)
    double split_age[2] = {-1, -1}; // seconds since each split was crossed (the addon's clock), -1 not yet
    int done_ms = -1;         // the last flying lap completed, -1 none
    int done_split[2] = {-1, -1};
    double done_age = -1;     // seconds since it was completed (the addon's clock), -1 none
    uint32_t laps = 0;        // flying laps begun this session (a new number: a new lap)
};

struct Snapshot {
    SessionIn s;
    MyLap my;
    std::vector<CarIn> cars;  // classification order, leader first
    std::vector<ChampEntry> champ; // a career's drivers' championship, the game's order (empty: none or unreadable)
};

// --------------------------------------------------------------------------------------------- config
struct Config {
    enum class Rows { all, top10, compact } rows = Rows::all;
    // autoswitch: as on TV (intervals, the leader gaps now and then, places gained after lap 1, names only on
    // lap 1 and under the safety car / red flag)
    enum class Gap { autoswitch, interval, leader, gained, pitstops, tyres } gap = Gap::autoswitch;
    // auto: codes beside data (gaps, times), full surnames when the data column is empty (as on TV)
    enum class Names { autoswitch, surname, code } names = Names::autoswitch;
    bool ban_leader = true;    // NEW RACE LEADER banner
    bool ban_fastest = true;   // FASTEST LAP banner (only with the game's own lap times)
    // practice and qualifying times (off: the core gives no lap times there and the game's counter does not move,
    // so the addon cannot time them reliably; on again once Swofty exposes the game's own best laps)
    bool timed_times = false;
    // the race's fastest lap (the purple icon on a row and its time flashing purple): off, it came from the addon's
    // own lap clock and did not always match the game's own fastest lap; on again with the game's lap times
    bool race_fastest = false;
    bool champ_formation = true; // the drivers' championship on the formation lap (careers, standings.hpp)
    bool gap_ms = false;       // race gaps in thousandths ("+0.934", earlier F1 games) instead of 2026's tenths
    bool tyres = true;
    bool tyre_age = false;
    bool arrows = true;
    bool highlight = true;     // player's row highlighted
    bool qcard = true;         // qualifying: the player's lap card (time, splits, target)
    // tv: the broadcast's team icons (private build; a team without one falls back to the game's badge);
    // game: the game's broadcast badges (colour, follow logo mods); game_white: the game's white HUD badges
    enum class Logos { tv, game, game_white } logos = Logos::game;
    int bg = 1;                // background: 0 solid, 1 broadcast (slightly translucent), 2 light
    bool en_practice = true, en_quali = true, en_sprint_quali = true, en_race = true;
    int compact_around = 1;    // compact: P1-P3 plus this many rows either side of the player
};

// ---------------------------------------------------------------------------------------------- outputs
// The banner top centre (races): NEW RACE LEADER, as on the broadcast. (FASTEST LAP was dropped in 0.15: the
// game gives addons no lap times, so ours never matched the game's own fastest-lap toast.)
enum Banner : int { banner_none = 0, banner_leader = 1, banner_fastest = 2 };

enum Card : int { card_none = 0, card_sc = 1, card_vsc = 2, card_red = 3, card_chq = 4, card_yellow = 5,
                  card_sc_end = 6 /* unused since 0.8.0: SC ENDING */, card_vsc_end = 7, card_clear = 8, card_sc_ending = 9 };

struct Row {
    std::string team_label;    // the constructors' table: the team's name in place of a driver's
    bool on = false;
    int vid = -1;
    std::string pos;
    bool p1 = false;
    bool up = false, dn = false;
    std::string gap;
    bool gap_purple = false;   // lap time flash: session best / new fastest lap
    bool gap_green = false;    // lap time flash: personal best
    bool gap_label = false;    // gap text is a label ("Interval", "Leader")
    bool gap_inpit = false;    // "IN PIT" in red (race)
    bool gain_on = false;      // places gained / lost view: an arrow (or a dash) before the number in `gap`
    int gain = 0;              // > 0 gained, < 0 lost, 0 none
    bool gap_champ = false;    // the championship's points: right-aligned where the tyre goes (room for long names)
    char tyre = 0;
    char start_tyre = 0;       // STARTING TYRES: the compound's name in its colour in the data column
    std::string tyre_age;
    bool pit = false;          // white P box
    bool pen = false;
    std::string pen_text;
    bool fl = false;           // fastest lap (race)
    bool chq = false;          // finished: chequered flag
    bool out = false;          // retired / DNF / DSQ / NC: dimmed
    bool ko = false;           // in the knockout zone
    bool ko_line = false;      // first row of the knockout zone: red line above
    bool elim = false;         // knocked out in an earlier segment (not shown since 0.2: kept for tests)
    bool sep = false;          // rows above are not contiguous (top10 / compact): separator line
    bool me = false;
    int team = -1;             // team identity (preview only)
    std::string badge;         // texture path for the game's own logo unit, "" none
    std::string name;          // demo only
};

struct Header {
    bool on = false;           // tower visible at all
    bool race = false, timed = false;
    std::string sess;          // "P1", "Q2", "SQ1"...
    std::string lap;           // race: the leader's lap
    std::string laps_total;    // demo only (else the game's binding)
    std::string clock;         // demo only (else the game's binding)
    bool demo = false;
    int card = card_none;
    std::string ysec;          // "SECTOR 2", "SECTORS 1 & 3", "ALL SECTORS"
    bool abbr = false;         // three-letter codes in the rows (else full surnames)
    bool bare = false;         // names only: no data column, no tyres (lap 1, safety car, red flag)
    bool gained = false;       // the "GAINED / LOST" view
    bool pitstops = false;     // the "PIT STOPS" view
    bool tyres = false;        // the "TYRES" view: laps on the current set
    bool start_tyres = false;  // "STARTING TYRES": on the grid and the formation lap
    bool final_cls = false;    // the winner has taken the flag: gaps at the line (the tower stays as on TV)
    bool formation = false;    // "FORMATION LAP" in place of the lap line
    bool champ = false;        // DRIVERS' CHAMPIONSHIP: the rows in the championship's order with their points
    bool cons = false;         // CONSTRUCTORS' CHAMPIONSHIP: one row a team (after the drivers', formation lap)
    bool wide = false;         // more than 22 rows shown (23-24: a My Team grid): slide the tower further up
    int banner = banner_none;  // the banner on screen, and its car
    int ban_vid = -1;
    int ban_team = -1;         // the car's team index (TV icons), -1 none
    std::string ban_badge;     // the game's badge texture path, "" none
    std::string ban_time;      // FASTEST LAP: the lap time
    std::string ban_name;      // the demo's driver name (in game the record reads the game's own name)
    std::string sub;           // the line under the logo: "SPRINT" or ""
    bool lead = false;         // gaps are to the leader
};

// The player's qualifying card, as the 2026 broadcast's (measured on the user's capture of the TV, 495 x 174 px):
// position, team, surname and tyre; the big figure is the lap running (tenths, grey until the first split, then
// white), or at a split the gap to the target for kSplitHold (green / purple / yellow) with "SECTOR n" under it; on
// the right the target's time to the end of the sector being run (its split, then the lap), and its name; S1 S2 S3
// bars in the sector's colour once done
enum SecState : int { sec_none = 0, sec_now = 1, sec_green = 2, sec_yellow = 3, sec_purple = 4 };
enum BigCol : int { big_white = 0, big_grey = 1, big_green = 2, big_yellow = 3, big_purple = 4 };
struct QCard {
    bool on = false;
    int vid = -1;
    int team = -1;
    std::string badge;         // the game's badge texture path
    std::string name;          // demo only
    std::string pos;           // "8", "-" without a time
    char tyre = 0;
    std::string big;           // "30.2" / "1:21.8" running, "-3.610" at a split, "1:21.845" a lap done
    int big_col = big_white;
    std::string target;        // the target's time to the end of the current sector, "" none
    int target_vid = -1;
    std::string target_name;   // demo only
    int sec[3] = {sec_none, sec_none, sec_none};
    bool sec_long[3] = {false, false, false}; // "SECTOR n" in place of "Sn" (its split just crossed)
};

// running lap time, tenths, truncated as on TV: "9.4", "56.6", "1:21.8"
inline std::string fmt_run(int ms)
{
    char b[24];
    if (ms < 0)
        ms = 0;
    if (ms >= 60000)
        std::snprintf(b, sizeof b, "%d:%02d.%d", ms / 60000, (ms / 1000) % 60, (ms % 1000) / 100);
    else
        std::snprintf(b, sizeof b, "%d.%d", ms / 1000, (ms % 1000) / 100);
    return b;
}

// a split's gap with its sign: "-0.457", "+0.696"
inline std::string fmt_delta(int ms)
{
    if (ms >= 0)
        return fmt_gap_ms(ms);
    std::string g = fmt_gap_ms(-ms);
    g[0] = '-';
    return g;
}

struct View {
    Header h;
    QCard card;
    std::array<Row, kRows> rows;
    int shown = 0;
};

// ------------------------------------------------------------------------------------------------ model
class Tower {
public:
    Tower() { reset_session(); }

    // now: seconds, monotonic (pauses included: flashes and arrows are wall-clock)
    const View &update(const Snapshot &snap_in, const Config &cfg, double now)
    {
        View &v = view_;
        v = View{};
        const Kind kind = kind_of(snap_in.s.type);

        if (snap_in.s.type != last_type_) {
            // Q1 -> Q2 -> Q3 (and SQ1 -> SQ2 -> SQ3) are the same weekend segment: keep the field size, or the
            // Q2 cut is computed from Q2's own (already-reduced) car count instead of the field that started Q1.
            bool same_segment = kind_of(last_type_) == kind && kind != Kind::none;
            reset_session(same_segment);
            last_type_ = snap_in.s.type;
        }

        // the red flag as the game reports it is only believed once it holds (a copy of the snapshot only while the
        // two differ, so the usual frame costs nothing)
        const Snapshot *sp = &snap_in;
        const bool red = red_filter(snap_in, kind, now);
        if (red != snap_in.s.red) {
            red_fixed_ = snap_in;
            red_fixed_.s.red = red;
            sp = &red_fixed_;
        }
        const Snapshot &snap = *sp;
        const SessionIn &s = snap.s;

        bool enabled = (kind == Kind::practice && cfg.en_practice) || (kind == Kind::quali && cfg.en_quali) ||
                       (kind == Kind::sprint_quali && cfg.en_sprint_quali) || (kind == Kind::race && cfg.en_race);
        if (!enabled || snap.cars.empty()) {
            seen_.fill(Seen{});
            return v;
        }

        Header &h = v.h;
        h.on = true;
        h.demo = s.demo;
        h.race = kind == Kind::race;
        h.timed = !h.race;
        h.sess = session_label(s.type);
        if (s.demo) {
            if (s.laps_total > 0)
                h.laps_total = std::to_string(s.laps_total);
            if (s.clock_s >= 0)
                h.clock = fmt_clock(s.clock_s);
        }

        const CarIn &lead = snap.cars.front();
        if (h.race) {
            int lap = lead.status == 3 ? lead.lap : lead.lap + 1;
            if (s.laps_total > 0 && lap > s.laps_total)
                lap = s.laps_total;
            if (lap < 1)
                lap = 1;
            h.lap = std::to_string(lap);
        }

        h.card = flag_card(s, h.race, now);
        if (kind == Kind::sprint_quali)
            h.sub = "SPRINT";
        h.ysec = yellow_text(s.yellow);
        h.lead = lead_mode(cfg, snap, now);
        if (h.race) {
            record_grid(snap);
            h.formation = s.formation;
            h.gained = gained_mode(cfg, snap, now);
            h.pitstops = !h.gained && pitstops_mode(cfg, snap, now);
            h.tyres = !h.gained && !h.pitstops && tyres_mode(cfg, snap, now);
            record_finish(snap, now);
            h.final_cls = s.chequered && lead.status == 3;
            // the formation lap, as the broadcast: the starting tyres first, then the drivers' championship for a while
            // (a career, its table read), then the names
            if (s.formation && formation_t0_ < 0)
                formation_t0_ = now;
            const double into = formation_t0_ >= 0 ? now - formation_t0_ : -1;
            const double after = s.demo ? 2.0 : kChampAfter, last = s.demo ? 10.0 : kChampSeconds; // the demo: shorter
            h.champ = cfg.champ_formation && s.formation && snap.champ.size() >= 8 && into >= after && into < after + last;
            // then the constructors' (the teams' sums of the game's driver points, when every scorer has a car here)
            const double cons_last = s.demo ? 6.0 : kConsSeconds;
            h.cons = cfg.champ_formation && s.formation && snap.champ.size() >= 8 && into >= after + last &&
                     into < after + last + cons_last && cons_table(snap, cons_);
            // before the start: the grid's tyres (as the broadcast shows them on the formation lap)
            h.start_tyres = cfg.gap == Config::Gap::autoswitch && cfg.tyres && !h.final_cls && !h.champ && on_grid(snap) &&
                            !(s.formation && snap.champ.size() >= 8 && cfg.champ_formation && into >= after);
            // TV: the safety car, the red flag and the formation lap show the names alone
            h.bare = cfg.gap == Config::Gap::autoswitch && !h.gained && !s.chequered &&
                     (s.sc == 1 || s.red || s.formation || h.start_tyres);
            if (h.bare || h.gained || h.pitstops || h.tyres)
                h.lead = false;
            if (h.final_cls)
                h.lead = true; // the final classification: gaps to the winner
        }
        // the starting tyres view shows data (the compound's name): codes, as the broadcast
        bool any_data = h.race ? (!h.bare || h.start_tyres) : false;
        if (!h.race && (cfg.timed_times || snap.s.game_times))
            for (const CarIn &c : snap.cars)
                any_data = any_data || c.best_ms > 0;
        h.abbr = cfg.names == Config::Names::code || (cfg.names == Config::Names::autoswitch && any_data && !h.champ);

        // field size for the knockout cut: the most racers seen in this session
        field_ = std::max(field_, std::max(s.racers, int(snap.cars.size())));

        if ((cfg.timed_times || snap.s.game_times))
            best_flash(snap, kind, now);
        fastest_flash(snap, kind, now);

        // qualifying: the knockout zone shows the gap to the last car through (as on TV)
        cut_best_ = -1;
        int seg = knockout_segment(s.type);
        if (seg == 1 || seg == 2) {
            int cut = knockout_cut(seg, field_);
            for (const CarIn &c : snap.cars)
                if (c.pos == cut)
                    cut_best_ = c.best_ms;
        }

        remember(snap, now);
        if (h.cons) {
            int slot = 0;
            for (const ConsEntry &e : cons_) {
                if (slot >= kRows)
                    break;
                Row &r = v.rows[size_t(slot++)];
                r = Row{};
                r.on = true;
                r.vid = -1;
                r.pos = std::to_string(e.rank);
                r.p1 = e.rank == 1;
                r.team = e.team;
                r.badge = e.badge.empty() ? "" : "#overrides.osd.timing_tower.team_badges_broadcast." + e.badge;
                r.team_label = team_label(e.team);
                r.gap = std::to_string(e.points);
                r.gap_champ = true;
            }
            v.shown = slot;
            banners(snap, kind, cfg, h, now);
            card(v.card, snap, kind, cfg);
            return v;
        }
        std::vector<int> pick = h.champ ? champ_rows(snap) : pick_rows(snap, cfg);
        int prev_idx = -1;
        int slot = 0;
        for (int idx : pick) {
            if (slot >= kRows)
                break;
            Row &r = v.rows[size_t(slot)];
            fill_row(r, snap, idx, kind, cfg, h, now);
            r.sep = prev_idx >= 0 && idx != prev_idx + 1;
            if (h.champ)
                champ_row(r, snap, idx);
            prev_idx = idx;
            slot++;
        }
        v.shown = slot;
        h.wide = slot > 22;
        banners(snap, kind, cfg, h, now);
        card(v.card, snap, kind, cfg);
        return v;
    }

    const View &view() const { return view_; }

    // keep_field: true on a Q1->Q2->Q3 (or SQ1->SQ2->SQ3) move, so the knockout cut still uses the field size
    // from the start of the weekend segment, not the smaller count left after a round's eliminations.
    void reset_session(bool keep_field = false)
    {
        seen_.fill(Seen{});
        if (!keep_field)
            field_ = 0;
        last_sc_ = 0;
        trans_card_ = card_none;
        trans_until_ = 0;
        lead_until_ = -1;
        last_leader_lap_ = -1;
        best_.fill(-1);
        flash_.fill(Flash{});
        started_ = false;
        fastest_ms_ = -1;
        fastest_flash_vid_ = -1;
        fastest_flash_until_ = 0;
        grid_.fill(0);
        grid_state_ = 0;
        red_since_ = -1;
        red_stale_ = false;
        last_pit_lap_ = -1;
        pit_until_ = -1;
        last_tyre_lap_ = -1;
        tyre_until_ = -1;
        finish_t_.fill(-1);
        gained_until_ = -1;
        gained_shown_ = false;
        leader_vid_ = -1;
        leader_cand_ = -1;
        formation_t0_ = -1;
        ban_ = Pending{};
        ban_until_ = -1;
        new_fastest_ = false;
        card_lap_ = ~0u;
        card_cool_ = false;
        card_tg_vid_ = -1;
        card_tg_cut_ = 0;
        ban_next_ = 0;
        queue_.clear();
    }

private:
    struct Seen {
        int pos = 0;
        double up_until = 0, dn_until = 0;
        bool valid = false;
    };
    struct Flash {
        double until = 0;
        bool purple = false;
        int ms = -1;
    };

    // ---- rows to show
    std::vector<int> pick_rows(const Snapshot &snap, const Config &cfg) const
    {
        const int n = int(snap.cars.size());
        int me = -1;
        for (int i = 0; i < n; i++)
            if (snap.cars[size_t(i)].me)
                me = i;
        std::vector<int> out;
        // Q2/Q3 (SQ2/SQ3): as on TV, the cars knocked out earlier leave the tower
        auto gone = [&](int i) { return knocked_out(snap.cars[size_t(i)], i, snap.s.type); };
        auto add = [&](int i) {
            if (i >= 0 && i < n && !gone(i) && std::find(out.begin(), out.end(), i) == out.end())
                out.push_back(i);
        };
        switch (cfg.rows) {
        case Config::Rows::all:
            for (int i = 0; i < n && i < kRows; i++)
                add(i);
            break;
        case Config::Rows::top10:
            for (int i = 0; i < n && int(out.size()) < 10; i++)
                add(i);
            add(me);
            break;
        case Config::Rows::compact: {
            for (int i = 0; i < n && int(out.size()) < 3; i++)
                add(i);
            if (me < 0 || gone(me)) {
                for (int i = 3; i < n && int(out.size()) < 6; i++)
                    add(i);
            } else {
                int k = cfg.compact_around < 0 ? 0 : cfg.compact_around;
                for (int i = me - k; i <= me + k; i++)
                    add(i);
            }
            break;
        }
        }
        std::sort(out.begin(), out.end());
        return out;
    }

    bool knocked_out(const CarIn &c, int idx, int type) const
    {
        int seg = knockout_segment(type);
        if (seg < 2)
            return false;
        int pos = c.pos > 0 ? c.pos : idx + 1;
        int field = std::max(field_, 1);
        int prev_cut = seg == 2 ? q2_entrants() : knockout_cut(2, field);
        return field > prev_cut && pos > prev_cut;
    }

    // Cars that took part in Q2/SQ2. The game may list the whole field (20, 22 or 24 cars: the ones knocked out in
    // Q1 leave the tower) or only the cars still in it (15, 16 or 17). A field of 17 or fewer is already Q2's own,
    // so nobody is cut from it, even when the tower never saw Q1 (Q1 skipped, or the game started at Q2).
    int q2_entrants() const
    {
        int field = std::max(field_, 1);
        return field <= 17 ? field : knockout_cut(1, field);
    }

    // ---- starting grid: the order through the formation lap (or the first frames on lap 0), frozen at the first
    // change of order once the formation lap is over. A race first seen past lap 1 has no grid.
    void record_grid(const Snapshot &snap)
    {
        const CarIn &lead = snap.cars.front();
        if (grid_state_ == 2)
            return;
        bool demo_grid = false;
        for (const CarIn &c : snap.cars)
            demo_grid = demo_grid || c.grid > 0;
        if (demo_grid) {
            for (const CarIn &c : snap.cars)
                if (c.vid < grid_.size())
                    grid_[c.vid] = c.grid;
            grid_state_ = 2;
            return;
        }
        auto take = [&] {
            for (const CarIn &c : snap.cars)
                if (c.vid < grid_.size())
                    grid_[c.vid] = c.pos;
        };
        if (grid_state_ == 0) {
            if (lead.lap > 0 && !snap.s.formation)
                return; // joined mid-race: no grid
            take();
            grid_state_ = 1;
            return;
        }
        if (snap.s.formation) {
            take();
            return;
        }
        bool changed = false;
        for (const CarIn &c : snap.cars)
            changed = changed || (c.vid < grid_.size() && grid_[c.vid] != c.pos);
        if (changed || lead.lap > 0)
            grid_state_ = 2; // lights out (the first change of order) or lap 1 done: the grid stays
    }

    bool gained_mode(const Config &cfg, const Snapshot &snap, double now)
    {
        if (grid_state_ != 2)
            return false;
        if (cfg.gap == Config::Gap::gained)
            return true;
        if (cfg.gap != Config::Gap::autoswitch)
            return false;
        // once, for 10 s, when the leader starts lap 2 (as the broadcast does early in a race)
        if (!gained_shown_ && snap.cars.front().lap >= 1 && snap.cars.front().lap <= 2) {
            gained_until_ = now + 10.0;
            gained_shown_ = true;
        }
        return now < gained_until_;
    }

    // ---- one row
    void fill_row(Row &r, const Snapshot &snap, int idx, Kind kind, const Config &cfg, const Header &h, double now)
    {
        const bool lead = h.lead;
        const CarIn &c = snap.cars[size_t(idx)];
        const CarIn &leader = snap.cars.front();
        const bool race = kind == Kind::race;
        r.on = true;
        r.vid = int(c.vid);
        int pos = c.pos > 0 ? c.pos : idx + 1;
        r.pos = std::to_string(pos);
        r.p1 = pos == 1;
        r.me = cfg.highlight && c.me;
        // the game's own badge for the car's team: the broadcast set is in colour (and follows logo mods)
        r.badge = badge_texture(c, cfg.logos == Config::Logos::game_white);
        r.team = team_index(c.badge);
        r.name = c.name.empty() ? "" : (h.abbr && !c.code.empty() ? c.code : c.name);

        const bool retired = c.status >= 4 && c.status <= 7;
        // timed sessions: when a segment ends every car still out reads 7; a car with a lap keeps its row
        r.out = race ? retired : (retired && c.best_ms < 0 && c.status != 7);
        r.chq = race && c.status == 3;

        // arrows
        if (cfg.arrows && race && c.vid < seen_.size()) { // TV shows place changes in races only
            const Seen &sn = seen_[c.vid];
            r.up = now < sn.up_until;
            r.dn = !r.up && now < sn.dn_until;
        }

        // tyre
        if (cfg.tyres && !r.out && (!h.bare || h.start_tyres) && !h.gained) {
            r.tyre = c.tyre;
            if (cfg.tyre_age && c.tyre && c.tyre_age >= 0)
                r.tyre_age = std::to_string(c.tyre_age);
        }

        // penalties
        if (c.drive_through) {
            r.pen = true;
            r.pen_text = "DT";
        } else if (c.stop_go) {
            r.pen = true;
            r.pen_text = "SG";
        } else if (c.pen_s > 0.05f) {
            r.pen = true;
            char b[12];
            std::snprintf(b, sizeof b, "+%ds", int(std::lround(c.pen_s)));
            r.pen_text = b;
        }

        if (race) {
            r.fl = (cfg.race_fastest || snap.s.game_times || snap.s.demo) && snap.s.fastest_vid >= 0 && uint32_t(snap.s.fastest_vid) == c.vid;
            if (h.bare && h.start_tyres) {
                // STARTING TYRES: "MEDIUM" in its colour where the gap goes, as the broadcast (not the letter)
                if (cfg.tyres && !retired && c.tyre)
                    r.start_tyre = c.tyre;
                r.tyre = 0;
                r.tyre_age.clear();
            } else if (h.bare) {
                // names only; under the safety car a car in the pits still shows it (stops are common then)
                if (snap.s.sc == 1 && !snap.s.red && !h.start_tyres && !snap.s.formation && !retired && c.in_pit &&
                    c.status != 3) {
                    r.gap = "IN PIT";
                    r.gap_inpit = true;
                    r.tyre = 0;
                    r.tyre_age.clear();
                }
            } else if (retired) {
                r.gap = c.status == 5 ? "DSQ" : c.status == 6 ? "NC" : "Out";
            } else if (snap.s.red) {
                r.gap = ""; // red flag: the field waits in the pit lane, no live gaps
            } else if (c.in_pit && c.status != 3) {
                // a car in the pits always shows it, even while GAINED/LOST, PIT STOPS or TYRES has the tower
                r.gap = "IN PIT";        // spans the data and tyre columns, as on the broadcast
                r.gap_inpit = true;
                r.tyre = 0;
                r.tyre_age.clear();
            } else if (h.gained) {
                int g = c.vid < grid_.size() ? grid_[c.vid] : 0;
                if (g > 0) {
                    r.gain = g - pos;
                    r.gain_on = true;
                    r.gap = std::to_string(r.gain < 0 ? -r.gain : r.gain);
                }
            } else if (h.pitstops) {
                if (c.pit_stops >= 0)
                    r.gap = std::to_string(c.pit_stops);
            } else if (h.tyres) {
                if (c.tyre_age >= 0)
                    r.gap = std::to_string(c.tyre_age);
            } else if (h.final_cls && c.status == 3) {
                // the gap at the line, from the moment each car took the flag
                double tw = winner_t();
                double tc = c.vid < finish_t_.size() ? finish_t_[c.vid] : -1;
                int down = leader.lap - c.lap;
                if (idx == 0) {
                    r.gap = "Leader";
                    r.gap_label = true;
                } else if (down > 0) {
                    r.gap = fmt_laps_down(down);
                } else if (tw >= 0 && tc >= tw) {
                    r.gap = fmt_gap_race(int(std::lround((tc - tw) * 1000.0)), cfg.gap_ms);
                }
            } else if (idx == 0) {
                r.gap = lead ? "Leader" : "Interval";
                r.gap_label = true;
            } else {
                int down = laps_down(snap, idx, lead ? 0 : idx - 1);
                int gap = lead ? c.gap_leader_ms : c.gap_ahead_ms;
                if (down > 0 && (lead || gap < 0))
                    r.gap = fmt_laps_down(down);
                else if (gap >= 0)
                    r.gap = fmt_gap_race(gap, cfg.gap_ms);
                else if (lead && c.gap_ahead_ms >= 0 && leader.status != 3)
                    r.gap = "";
            }
            // a new fastest lap flashes the lap time in purple
            if ((cfg.race_fastest || snap.s.game_times || snap.s.demo) && !h.bare && !h.gained && !h.pitstops && !h.tyres && fastest_flash_vid_ == int(c.vid) && now < fastest_flash_until_ &&
                fastest_ms_ > 0) {
                r.gap = fmt_lap(fastest_ms_);
                r.gap_purple = true;
                r.gap_label = false;
                r.gap_inpit = false;
            }
        } else {
            r.pit = c.in_pit;
            int best = c.best_ms;
            int lead_best = leader.best_ms;
            // knockout zone
            int seg = knockout_segment(snap.s.type);
            if (seg == 1 || seg == 2) {
                int cut = knockout_cut(seg, field_);
                int prev_cut = seg == 2 ? q2_entrants() : field_;
                if (pos > prev_cut)
                    r.elim = true;
                else if (pos > cut && !r.out) {
                    r.ko = true;
                    r.ko_line = pos == cut + 1;
                }
            } else if (seg == 3) {
                if (pos > 10)
                    r.elim = true;
            }
            // P1 its lap time; the rest the gap to P1; the knockout zone the gap to the last car through
            if (best > 0 && (cfg.timed_times || snap.s.game_times)) {
                if (idx == 0 || lead_best <= 0)
                    r.gap = fmt_lap(best);
                else if (r.ko && cut_best_ > 0 && best >= cut_best_)
                    r.gap = fmt_gap_ms(best - cut_best_);
                else
                    r.gap = fmt_gap_ms(best - lead_best);
            }
            // as the broadcast's qualifying tower (the user's capture): "Out Lap" for a car on its out lap (P1 keeps
            // its time), and "-" for its place and no tyre for a car without a time. The broadcast writes "No Time"
            // there, but in our wider font it ran over every long surname (0.23.0 in game, Q2 before any lap): blank
            if (cfg.timed_times || snap.s.game_times) {
                if (best <= 0) {
                    r.gap.clear();
                    r.pos = "-";
                    r.p1 = false;
                    r.tyre = 0;
                } else if (c.out_lap && idx != 0) {
                    r.gap = "Out Lap";
                }
            }
            if ((cfg.timed_times || snap.s.game_times) && c.vid < flash_.size() && now < flash_[c.vid].until && flash_[c.vid].ms > 0) {
                r.gap = fmt_lap(flash_[c.vid].ms);
                r.gap_purple = flash_[c.vid].purple;
                r.gap_green = !flash_[c.vid].purple;
            }
        }
    }

    // Whole laps car `idx` is behind car `ref`: by distance when the session distance and the track length are
    // known (a car one lap counter behind but less than a lap of distance is not lapped), else by lap counters
    // once the reference car has finished.
    static int laps_down(const Snapshot &snap, int idx, int ref)
    {
        const CarIn &c = snap.cars[size_t(idx)];
        const CarIn &a = snap.cars[size_t(ref)];
        if (a.status == 3 && c.status == 3)
            return 0;
        if (a.have_total && c.have_total && snap.s.track_m > 100.f) {
            float d = (a.total_m - c.total_m) / snap.s.track_m;
            return d >= 1.f ? int(d) : 0;
        }
        int d = a.lap - c.lap;
        return d > 1 ? d - 1 : 0;
    }

    // ---- header card with transitions (VSC ENDING, TRACK CLEAR) and priorities
    int flag_card(const SessionIn &s, bool race, double now)
    {
        int sc = race ? s.sc : 0;
        if (last_sc_ == 2 && sc == 0 && !s.red) {
            trans_card_ = card_vsc_end;    // "VSC" / "ENDING" for 4 s, then TRACK CLEAR
            trans_until_ = now + 4.0;
            clear_at_ = now + 4.0;
        } else if (last_sc_ == 1 && sc == 0 && !s.red) {
            trans_card_ = card_sc_ending;  // "SC" / "ENDING" for 4 s, then TRACK CLEAR (same shape as the VSC one)
            trans_until_ = now + 4.0;
            clear_at_ = now + 4.0;
        }
        if (sc != 0) {
            trans_card_ = card_none;
            clear_at_ = -1;
        }
        last_sc_ = sc;
        if ((trans_card_ == card_vsc_end || trans_card_ == card_sc_ending) && now >= trans_until_ && clear_at_ >= 0) {
            trans_card_ = card_clear;
            trans_until_ = now + 4.0;
            clear_at_ = -1;
        }

        // the chequered flag: no header block on the broadcast, each finisher gets its flag on its row
        if (race && s.chequered)
            return card_none;
        if (s.red)
            return card_red;
        if (sc == 1)
            // the safety car heads into the pit lane: "SC" / "ENDING" (the engineer's "safety car in this lap"; the
            // game gives no earlier sign), kept 4 s past the restart and then TRACK CLEAR
            return s.sc_in ? card_sc_ending : card_sc;
        if (sc == 2)
            return card_vsc;
        if (trans_card_ != card_none && now < trans_until_)
            return trans_card_;
        if (s.yellow)
            return card_yellow;
        return card_none;
    }

    static std::string yellow_text(uint8_t bits)
    {
        std::vector<int> s;
        for (int i = 0; i < 8; i++)
            if (bits & (1u << i))
                s.push_back(i + 1);
        if (s.empty())
            return "";
        // three or more do not fit the block ("SECTORS 1, 2 & 3" ran past its edge)
        if (s.size() >= 3)
            return s.size() == 3 && s.front() == 1 && s.back() == 3
                       ? "ALL SECTORS"
                       : "SECTORS " + std::to_string(s.front()) + "-" + std::to_string(s.back());
        std::string t = s.size() == 1 ? "SECTOR " : "SECTORS ";
        for (size_t i = 0; i < s.size(); i++) {
            if (i)
                t += i + 1 == s.size() ? " & " : ", ";
            t += std::to_string(s[i]);
        }
        return t;
    }

    // ---- pit stops view: fixed, or (auto) 10 s each time the leader starts a lap that is a multiple of 10 once
    // a car has stopped (the leader view takes the other multiples of 5)
    // ---- tyres view: fixed, or (auto) 10 s when the leader starts laps 7, 17, 27...
    bool tyres_mode(const Config &cfg, const Snapshot &snap, double now)
    {
        if (cfg.gap == Config::Gap::tyres)
            return true;
        if (cfg.gap != Config::Gap::autoswitch || !cfg.tyres)
            return false;
        int ll = snap.cars.front().lap;
        if (last_tyre_lap_ >= 0 && ll != last_tyre_lap_ && ll >= 7 && (ll % 10) == 7)
            tyre_until_ = now + 10.0;
        last_tyre_lap_ = ll;
        return now < tyre_until_;
    }

    // ---- on the grid: lap 0, before the order first changes after the formation lap, and every car standing
    // (or the formation lap itself)
    // A red flag seen by a user at the start of a race (on the grid, before anything happened) came with the rest of
    // the tower in its red-flag look. So the game's flag counts only after kRedHoldSeconds in a row and, in a race,
    // once the leader is away (past kRedAwayM or on lap 2); one already up on the grid or the formation lap is
    // ignored until the game drops it.
    static constexpr double kRedHoldSeconds = 2.0;
    static constexpr float kRedAwayM = 500.f;
    bool red_filter(const Snapshot &snap, Kind kind, double now)
    {
        if (!snap.s.red || snap.cars.empty()) {
            red_since_ = -1;
            red_stale_ = false;
            return false;
        }
        if (snap.s.demo)
            return true;
        if (kind == Kind::race) {
            const CarIn &lead = snap.cars.front();
            const bool away = lead.lap >= 1 || (lead.have_total && lead.total_m > kRedAwayM);
            if (snap.s.formation || !away || on_grid(snap))
                red_stale_ = true;
        }
        if (red_stale_)
            return false;
        if (red_since_ < 0)
            red_since_ = now;
        return now - red_since_ >= kRedHoldSeconds;
    }

    bool on_grid(const Snapshot &snap) const
    {
        const CarIn &lead = snap.cars.front();
        if (lead.lap != 0 || grid_state_ == 2)
            return false;
        if (snap.s.formation)
            return true;
        bool known = false;
        for (const CarIn &c : snap.cars) {
            if (c.speed_kmh < 0)
                continue;
            known = true;
            if (c.speed_kmh > 3.f)
                return false;
        }
        return known;
    }

    void record_finish(const Snapshot &snap, double now)
    {
        for (const CarIn &c : snap.cars)
            if (c.status == 3 && c.vid < finish_t_.size() && finish_t_[c.vid] < 0)
                finish_t_[c.vid] = now;
    }

    double winner_t() const
    {
        double t = -1;
        for (double x : finish_t_)
            if (x >= 0 && (t < 0 || x < t))
                t = x;
        return t;
    }

    bool pitstops_mode(const Config &cfg, const Snapshot &snap, double now)
    {
        if (cfg.gap == Config::Gap::pitstops)
            return true;
        if (cfg.gap != Config::Gap::autoswitch)
            return false;
        int ll = snap.cars.front().lap;
        bool stopped = false;
        for (const CarIn &c : snap.cars)
            stopped = stopped || c.pit_stops > 0;
        if (stopped && last_pit_lap_ >= 0 && ll != last_pit_lap_ && ll > 0 && (ll % 10) == 0)
            pit_until_ = now + 10.0;
        last_pit_lap_ = ll;
        return now < pit_until_;
    }

    // ---- interval / leader
    bool lead_mode(const Config &cfg, const Snapshot &snap, double now)
    {
        if (cfg.gap == Config::Gap::leader)
            return true;
        if (cfg.gap == Config::Gap::interval || kind_of(snap.s.type) != Kind::race)
            return false;
        int ll = snap.cars.front().lap;
        if (last_leader_lap_ >= 0 && ll != last_leader_lap_ && ll > 0 && (ll % 5) == 0 && (ll % 10) != 0)
            lead_until_ = now + 8.0;
        last_leader_lap_ = ll;
        return now < lead_until_;
    }

    // ---- personal / session best flashes (timed sessions)
    void best_flash(const Snapshot &snap, Kind kind, double now)
    {
        if (kind == Kind::race || kind == Kind::none)
            return;
        int session_best = -1;
        for (const CarIn &c : snap.cars)
            if (c.vid < best_.size() && best_[c.vid] > 0 && (session_best < 0 || best_[c.vid] < session_best))
                session_best = best_[c.vid];
        for (const CarIn &c : snap.cars) {
            if (c.vid >= best_.size() || c.best_ms <= 0)
                continue;
            int old = best_[c.vid];
            if (old < 0 || c.best_ms < old) {
                if (old >= 0 || started_) { // no flash for the times already there when we start
                    Flash &f = flash_[c.vid];
                    f.until = now + 3.0;
                    f.ms = c.best_ms;
                    f.purple = session_best < 0 || c.best_ms < session_best;
                }
                best_[c.vid] = c.best_ms;
            }
        }
        started_ = true;
    }

    void fastest_flash(const Snapshot &snap, Kind kind, double now)
    {
        if (kind != Kind::race)
            return;
        if (snap.s.fastest_ms > 0 && (fastest_ms_ < 0 || snap.s.fastest_ms < fastest_ms_)) {
            // every new fastest lap of the race, the first too (as the game's own toast); a first one met in a race
            // already well under way (the tower joining late) is only taken in
            const int lead_lap = snap.cars.empty() ? 0 : snap.cars.front().lap;
            if (fastest_ms_ > 0 || snap.s.demo || lead_lap <= 2) {
                fastest_flash_vid_ = snap.s.fastest_vid;
                fastest_flash_until_ = now + 5.0;
                new_fastest_ = snap.s.game_times || snap.s.demo; // the banner: only with the game's own times
            }
            fastest_ms_ = snap.s.fastest_ms;
        }
    }

    // ---- banner: NEW RACE LEADER when the lead changes hands from lap 2 (held 1 s, so a re-sort or a torn read
    // does not count), for kBannerSeconds; a change while one shows follows it after a short gap. Races only.
    static constexpr double kBannerSeconds = 6.0;
    static constexpr double kBannerGap = 0.6;
    static constexpr double kChampAfter = 20.0;   // the formation lap: starting tyres for this long, then...
    static constexpr double kChampSeconds = 30.0; // ...the drivers' championship for this long
    static constexpr double kConsSeconds = 20.0;  // ...then the constructors' for this long
    static constexpr int kLeaderClearMs = 600; // NEW RACE LEADER once the new leader is this far clear of P2
    static constexpr double kLeaderHoldSeconds = 4.0; // ...or once it has led this long, however close P2 is

    void banners(const Snapshot &snap, Kind kind, const Config &cfg, Header &h, double now)
    {
        if (kind != Kind::race || snap.cars.empty()) {
            leader_vid_ = -1;
            leader_cand_ = -1;
            queue_.clear();
            ban_until_ = -1;
            return;
        }
        const SessionIn &s = snap.s;
        const CarIn &lead = snap.cars.front();
        const int v = int(lead.vid);
        const bool racing = !s.formation && !s.chequered && !s.red && !h.start_tyres && lead.lap >= 1;
        if (leader_vid_ < 0 || !racing) {
            leader_vid_ = v; // first sight, the start, a red flag: no banner, just follow
            leader_cand_ = -1;
        } else if (v != leader_vid_) {
            // a new leader is announced once it has pulled 0.6 s clear of P2 after at least 1 s in front (a re-sort
            // or a torn read does not count), or after kLeaderHoldSeconds in front however close P2 is (a user: the
            // banner never came while the two stayed close); if the old leader takes it back first, nothing is said
            const int gap2 = snap.cars.size() > 1 ? snap.cars[1].gap_ahead_ms : -1;
            if (leader_cand_ != v) {
                leader_cand_ = v;
                leader_cand_t_ = now;
            }
            const double held = now - leader_cand_t_;
            if ((held >= 1.0 && gap2 >= kLeaderClearMs) || held >= kLeaderHoldSeconds) {
                if (cfg.ban_leader)
                    push(Pending{banner_leader, v});
                leader_vid_ = v;
                leader_cand_ = -1;
            }
        } else {
            leader_cand_ = -1;
        }
        if (new_fastest_) {
            new_fastest_ = false;
            if (cfg.ban_fastest && !s.chequered && s.fastest_vid >= 0)
                push(Pending{banner_fastest, s.fastest_vid, s.fastest_ms});
        }
        if (ban_until_ >= 0 && now >= ban_until_) {
            ban_until_ = -1;
            ban_next_ = now + kBannerGap;
        }
        if (ban_until_ < 0 && !queue_.empty() && now >= ban_next_) {
            ban_ = queue_.front();
            queue_.erase(queue_.begin());
            ban_until_ = now + kBannerSeconds;
        }
        if (ban_until_ < 0 || (ban_.kind == banner_leader && !cfg.ban_leader) ||
            (ban_.kind == banner_fastest && !cfg.ban_fastest))
            return;
        h.banner = ban_.kind;
        h.ban_vid = ban_.vid;
        if (ban_.kind == banner_fastest)
            h.ban_time = fmt_lap(ban_.ms);
        for (const CarIn &c : snap.cars)
            if (int(c.vid) == ban_.vid) {
                h.ban_team = team_index(c.badge);
                h.ban_name = c.name;
                h.ban_badge = badge_texture(c, false);
            }
    }

    // ---- the qualifying card
    static constexpr double kSplitHold = 4.5; // a split's gap stays this long in place of the time (TV: ~4.5 s)
    static constexpr double kDoneDelta = 4.5; // a lap done: its gap, then the lap time until kDoneHold
    static constexpr double kDoneHold = 8.0;

    void card(QCard &k, const Snapshot &snap, Kind kind, const Config &cfg)
    {
        const MyLap &m = snap.my;
        k = QCard{};
        if (!cfg.qcard || !(kind == Kind::quali || kind == Kind::sprint_quali) || !m.known)
            return;
        const CarIn *me = nullptr;
        for (const CarIn &c : snap.cars)
            if (c.me)
                me = &c;
        if (!me)
            return;
        const bool done = m.done_ms > 0 && m.done_age >= 0 && m.done_age < kDoneHold;
        if (m.laps != card_lap_) {
            card_lap_ = m.laps;
            card_cool_ = false;
        }

        // the target: the fastest other car; in a knockout zone (or without a time) the last car through. It follows
        // the order during the lap (on TV it changed between two splits when P1 changed), and stays put while a lap
        // just done is on show
        const int seg = knockout_segment(snap.s.type);
        const int cut = (seg == 1 || seg == 2) ? knockout_cut(seg, field_) : 0;
        const CarIn *p1 = nullptr, *cut_car = nullptr;
        for (const CarIn &c : snap.cars) {
            if (c.me || c.best_ms <= 0)
                continue;
            if (!p1 || c.best_ms < p1->best_ms)
                p1 = &c;
            if (cut > 0 && c.pos == cut)
                cut_car = &c;
        }
        const bool through = me->best_ms > 0 && me->pos > 0 && me->pos <= cut;
        const CarIn *pick = cut > 0 && cut_car && !through ? cut_car : p1;
        if (!done || card_tg_vid_ < 0) {
            card_tg_vid_ = pick ? int(pick->vid) : -1;
            card_tg_cut_ = pick && pick == cut_car && pick != p1 ? cut : 0;
        }
        const CarIn *tg = nullptr;
        for (const CarIn &c : snap.cars)
            if (int(c.vid) == card_tg_vid_)
                tg = &c;
        const bool tg_cut = tg && card_tg_cut_ > 0;

        // the session's best of each sector (every car's own best)
        int sb[3] = {-1, -1, -1};
        for (const CarIn &c : snap.cars)
            for (int i = 0; i < 3; i++)
                if (c.pb_sec[i] > 0 && (sb[i] < 0 || c.pb_sec[i] < sb[i]))
                    sb[i] = c.pb_sec[i];
        auto colour = [&](int i, int st) {
            if (st <= 0)
                return int(sec_none);
            if (sb[i] > 0 && st <= sb[i])
                return int(sec_purple);
            if (me->pb_sec[i] <= 0 || st <= me->pb_sec[i])
                return int(sec_green);
            return int(sec_yellow);
        };
        // under the target: green (purple when the target is P1: a provisional fastest); over it: yellow
        auto gap_col = [&](int d) { return d < 0 ? (tg_cut ? int(big_green) : int(big_purple)) : int(big_yellow); };
        auto tsum = [&](int n) { // the target lap to the end of sector n (0-based), -1 unknown
            if (!tg)
                return -1;
            if (n >= 2)
                return tg->best_ms;
            int t = 0;
            for (int i = 0; i <= n; i++) {
                if (tg->best_sec[i] <= 0)
                    return -1;
                t += tg->best_sec[i];
            }
            return t;
        };
        int tshow = -1; // the target's time on the right

        if (done) {
            // the lap just done: its gap with SECTOR 3, then its time, until kDoneHold (the next lap shows after)
            const int s1 = m.done_split[0];
            const int s2 = m.done_split[1] > 0 && s1 > 0 ? m.done_split[1] - s1 : -1;
            const int s3 = m.done_split[1] > 0 ? m.done_ms - m.done_split[1] : -1;
            k.sec[0] = colour(0, s1);
            k.sec[1] = colour(1, s2);
            k.sec[2] = colour(2, s3);
            tshow = tsum(2);
            if (m.done_age < kDoneDelta && tshow > 0) {
                const int d = m.done_ms - tshow;
                k.big = fmt_delta(d);
                k.big_col = gap_col(d);
                k.sec_long[2] = true;
            } else {
                k.big = fmt_lap(m.done_ms);
            }
        } else if (m.flying && m.t_ms >= 0) {
            // an in-lap or a cool-down lap (sector 1 over 3 s off the car's best, or a quarter past a lap): no card
            if (m.split[0] > 0 && me->pb_sec[0] > 0 && m.split[0] > me->pb_sec[0] + 3000)
                card_cool_ = true;
            const int ref = me->best_ms > 0 ? me->best_ms : tg ? tg->best_ms : -1;
            if (ref > 0 && m.t_ms > ref + ref / 4)
                card_cool_ = true;
            if (card_cool_)
                return;
            const int s1 = m.split[0], s2 = m.split[1] > 0 && s1 > 0 ? m.split[1] - s1 : -1;
            k.sec[0] = s1 > 0 ? colour(0, s1) : int(sec_now);
            k.sec[1] = s2 > 0 ? colour(1, s2) : s1 > 0 ? int(sec_now) : int(sec_none);
            k.sec[2] = s2 > 0 ? int(sec_now) : int(sec_none);
            // a split just crossed: its gap in place of the time, the target still at that split
            const int at = m.split[1] > 0 ? 1 : m.split[0] > 0 ? 0 : -1;
            const bool holding = at >= 0 && m.split_age[at] >= 0 && m.split_age[at] < kSplitHold && tsum(at) > 0;
            if (holding) {
                tshow = tsum(at);
                const int d = m.split[at] - tshow;
                k.big = fmt_delta(d);
                k.big_col = gap_col(d);
                k.sec_long[at] = true;
            } else {
                k.big = fmt_run(m.t_ms);
                k.big_col = s1 > 0 ? int(big_white) : int(big_grey);
                tshow = tsum(at + 1);
            }
        } else {
            return;
        }
        k.on = true;
        k.vid = int(me->vid);
        k.team = team_index(me->badge);
        k.badge = badge_texture(*me, false);
        k.name = me->name;
        k.pos = me->best_ms > 0 && me->pos > 0 ? std::to_string(me->pos) : "-";
        k.tyre = me->tyre;
        if (tg && tshow > 0) {
            k.target = fmt_lap_short(tshow);
            k.target_vid = int(tg->vid);
            k.target_name = tg->name;
        }
    }

    uint32_t card_lap_ = ~0u;
    bool card_cool_ = false;
    int card_tg_vid_ = -1, card_tg_cut_ = 0;

    struct Pending {
        int kind = banner_none;
        int vid = -1;
        int ms = 0;
    };

    // a newer banner of the same kind replaces the one waiting (the latest leader, the latest fastest lap)
    void push(const Pending &p)
    {
        for (Pending &q : queue_)
            if (q.kind == p.kind) {
                q = p;
                return;
            }
        queue_.push_back(p);
    }

    // ---- the constructors' championship: each team's points, the sum of its drivers' in the game's own drivers'
    // table, by the team of each driver's car in this session. Only when every driver with points has a car here and
    // every car's team is known (a reserve who scored, or a driver moved, would put points in the wrong place): else
    // false and the formation lap goes on without it. Ties: the team with the better-placed driver first.
    struct ConsEntry {
        int team = -1;
        int points = 0;
        int best = 0;   // its best driver's place in the drivers' table
        int rank = 0;
        std::string badge;
    };
    std::vector<ConsEntry> cons_;

    static bool cons_table(const Snapshot &snap, std::vector<ConsEntry> &out)
    {
        out.clear();
        std::array<int, kTeams> at;
        at.fill(-1);
        for (const ChampEntry &e : snap.champ) {
            const CarIn *car = nullptr;
            for (const CarIn &c : snap.cars)
                if (e.vid >= 0 && int(c.vid) == e.vid)
                    car = &c;
            if (!car)
                if (e.points > 0)
                    return false; // points scored by a driver not here
                else
                    continue;
            const int t = team_index(car->badge);
            if (t < 0)
                return false;     // a team we cannot name (My Team, a retro team)
            if (at[size_t(t)] < 0) {
                at[size_t(t)] = int(out.size());
                ConsEntry ce;
                ce.team = t;
                ce.best = e.rank;
                ce.badge = car->badge;
                out.push_back(ce);
            }
            ConsEntry &ce = out[size_t(at[size_t(t)])];
            ce.points += e.points;
            if (e.rank < ce.best)
                ce.best = e.rank;
        }
        if (out.size() < 5)
            return false;
        std::stable_sort(out.begin(), out.end(), [](const ConsEntry &a, const ConsEntry &b) {
            return a.points != b.points ? a.points > b.points : a.best < b.best;
        });
        for (size_t i = 0; i < out.size(); i++)
            out[i].rank = int(i) + 1;
        return true;
    }

    // ---- the drivers' championship: the cars of this session in the table's order (a driver without a car here, a
    // reserve or a replaced one, is left out; the rest keep their real place)
    std::vector<int> champ_rows(const Snapshot &snap) const
    {
        std::vector<int> out;
        for (const ChampEntry &e : snap.champ) {
            if (e.vid < 0)
                continue;
            for (size_t i = 0; i < snap.cars.size(); i++)
                if (int(snap.cars[i].vid) == e.vid) {
                    out.push_back(int(i));
                    break;
                }
        }
        return out;
    }

    // a championship row: its place and points in the table, the full surname and the team, nothing else
    static void champ_row(Row &r, const Snapshot &snap, int idx)
    {
        const CarIn &c = snap.cars[size_t(idx)];
        for (const ChampEntry &e : snap.champ)
            if (e.vid == int(c.vid)) {
                r.pos = std::to_string(e.rank);
                r.p1 = e.rank == 1;
                r.gap = std::to_string(e.points);
                r.gap_champ = true;
                break;
            }
        r.sep = false;
        r.up = r.dn = false;
        r.ko = r.ko_line = r.elim = r.out = false;
        r.gap_purple = r.gap_green = r.gap_label = r.gap_inpit = false;
        r.tyre = 0;
        r.start_tyre = 0;
        r.tyre_age.clear();
        r.pit = r.pen = r.fl = r.chq = false;
        r.gain_on = false;
    }

    void remember(const Snapshot &snap, double now)
    {
        for (const CarIn &c : snap.cars) {
            if (c.vid >= seen_.size() || c.pos <= 0)
                continue;
            Seen &sn = seen_[c.vid];
            if (sn.valid && sn.pos != c.pos) {
                if (c.pos < sn.pos)
                    sn.up_until = now + 3.0, sn.dn_until = 0;
                else
                    sn.dn_until = now + 3.0, sn.up_until = 0;
            }
            sn.pos = c.pos;
            sn.valid = true;
        }
    }

    View view_;
    std::array<Seen, 32> seen_{};
    std::array<int, 32> best_{};
    std::array<Flash, 32> flash_{};
    bool started_ = false;
    int field_ = 0;
    int last_type_ = -1;
    int last_sc_ = 0;
    int trans_card_ = card_none;
    double trans_until_ = 0, clear_at_ = -1;
    double lead_until_ = -1;
    int last_leader_lap_ = -1;
    int fastest_ms_ = -1;
    int fastest_flash_vid_ = -1;
    double fastest_flash_until_ = 0;
    int leader_vid_ = -1, leader_cand_ = -1;
    double formation_t0_ = -1;
    double leader_cand_t_ = 0;
    Pending ban_;
    bool new_fastest_ = false;
    double ban_until_ = -1, ban_next_ = 0;
    std::vector<Pending> queue_;
    std::array<int, 32> grid_{};
    int cut_best_ = -1;
    int last_pit_lap_ = -1;
    int last_tyre_lap_ = -1;
    double tyre_until_ = -1;
    std::array<double, 32> finish_t_ = minus_ones();
    static std::array<double, 32> minus_ones()
    {
        std::array<double, 32> a{};
        a.fill(-1);
        return a;
    }
    double pit_until_ = -1;
    int grid_state_ = 0;       // 0 none, 1 recording, 2 frozen
    double red_since_ = -1;    // the game's red flag up since (red_filter)
    bool red_stale_ = false;   // ...already up before the race got going: ignored until it drops
    Snapshot red_fixed_;
    double gained_until_ = -1;
    bool gained_shown_ = false;
};

} // namespace tvt
