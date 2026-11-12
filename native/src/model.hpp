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

constexpr int kRows = 22;    // rows the HUD record composes
constexpr int kSlots = 24;   // per-car name slots per row (#session.cars.car0 ... car23)
constexpr int kTeams = 12;   // private team logo sprites

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
// 2026 broadcast: one decimal, truncated ("+0.9"); over a minute "+1:05.3".
inline std::string fmt_gap_race(int ms)
{
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

inline std::string fmt_laps_down(int n)
{
    if (n <= 0)
        return "";
    char b[16];
    std::snprintf(b, sizeof b, "+%d LAP%s", n, n == 1 ? "" : "S");
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
// Private logo sprites, one per team identity. The game's badge keys look like "ferrari_2026"; we match by
// substring so new keys of the same team keep working.
inline const char *team_name(int i)
{
    static const char *n[kTeams] = {"mercedes", "ferrari", "redbull", "mclaren", "astonmartin", "alpine",
                                    "williams", "racingbulls", "haas", "sauber", "audi", "cadillac"};
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
    int best_ms = -1;         // best valid lap
    int last_ms = -1;         // last completed lap
    bool me = false;
    std::string badge;        // the game's team badge key ("ferrari_2026") or ""
    std::string name;         // only demo mode: the driver's surname
    std::string code;         // only demo mode: three-letter code
};

struct SessionIn {
    int type = 0;
    int racers = 0;           // competitors in the session (not the safety car)
    float track_m = 0;        // track length
    int sc = 0;               // 0 none, 1 safety car, 2 virtual
    bool sc_in = false;       // the safety car is coming in (inferred)
    bool red = false;
    uint8_t yellow = 0;       // bit i: sector i+1
    bool chequered = false;
    int laps_total = -1;      // -1: take it from the game binding
    double clock_s = -1;      // -1: take it from the game binding
    int fastest_vid = -1;     // race fastest lap holder
    int fastest_ms = -1;
    bool demo = false;
};

struct Snapshot {
    SessionIn s;
    std::vector<CarIn> cars;  // classification order, leader first
};

// --------------------------------------------------------------------------------------------- config
struct Config {
    enum class Rows { all, top10, compact } rows = Rows::all;
    enum class Gap { autoswitch, interval, leader } gap = Gap::autoswitch;
    bool abbr = false;         // three-letter codes instead of surnames
    bool tyres = true;
    bool tyre_age = false;
    bool arrows = true;
    bool colbar = false;       // team colour bar
    bool highlight = true;     // player's row highlighted
    int bg = 1;                // background: 0 solid, 1 broadcast (slightly translucent), 2 light
    bool en_practice = true, en_quali = true, en_sprint_quali = true, en_race = true;
    int compact_around = 1;    // compact: P1-P3 plus this many rows either side of the player
};

// ---------------------------------------------------------------------------------------------- outputs
enum Card : int { card_none = 0, card_sc = 1, card_vsc = 2, card_red = 3, card_chq = 4, card_yellow = 5,
                  card_sc_end = 6, card_vsc_end = 7, card_clear = 8 };

struct Row {
    bool on = false;
    int vid = -1;
    std::string pos;
    bool p1 = false;
    bool up = false, dn = false;
    std::string gap;
    bool gap_purple = false;   // lap time flash: session best / new fastest lap
    bool gap_green = false;    // lap time flash: personal best
    bool gap_label = false;    // gap text is a label ("INTERVAL", "LEADER")
    char tyre = 0;
    std::string tyre_age;
    bool pit = false;          // white P box
    bool pen = false;
    std::string pen_text;
    bool fl = false;           // fastest lap (race)
    bool chq = false;          // finished: chequered flag
    bool out = false;          // retired / DNF / DSQ / NC: dimmed
    bool ko = false;           // in the knockout zone
    bool ko_line = false;      // first row of the knockout zone: red line above
    bool elim = false;         // knocked out in an earlier segment: dimmed
    bool sep = false;          // rows above are not contiguous (top10 / compact): separator line
    bool me = false;
    int team = -1;             // private logo index
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
    std::string ysec;          // "S2", "S1 S3"
    bool lead = false;         // gaps are to the leader
};

struct View {
    Header h;
    std::array<Row, kRows> rows;
    int shown = 0;
};

// ------------------------------------------------------------------------------------------------ model
class Tower {
public:
    Tower() { reset_session(); }

    // now: seconds, monotonic (pauses included: flashes and arrows are wall-clock)
    const View &update(const Snapshot &snap, const Config &cfg, double now)
    {
        View &v = view_;
        v = View{};
        const SessionIn &s = snap.s;
        const Kind kind = kind_of(s.type);

        if (s.type != last_type_) {
            reset_session();
            last_type_ = s.type;
        }

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
        h.ysec = yellow_text(s.yellow);
        h.lead = lead_mode(cfg, snap, now);

        // field size for the knockout cut: the most racers seen in this session
        field_ = std::max(field_, std::max(s.racers, int(snap.cars.size())));

        best_flash(snap, kind, now);
        fastest_flash(snap, kind, now);

        remember(snap, now);
        std::vector<int> pick = pick_rows(snap, cfg);
        int prev_idx = -1;
        int slot = 0;
        for (int idx : pick) {
            if (slot >= kRows)
                break;
            Row &r = v.rows[size_t(slot)];
            fill_row(r, snap, idx, kind, cfg, h.lead, now);
            r.sep = prev_idx >= 0 && idx != prev_idx + 1;
            prev_idx = idx;
            slot++;
        }
        v.shown = slot;
        return v;
    }

    const View &view() const { return view_; }

    void reset_session()
    {
        seen_.fill(Seen{});
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
        auto add = [&](int i) {
            if (i >= 0 && i < n && std::find(out.begin(), out.end(), i) == out.end())
                out.push_back(i);
        };
        switch (cfg.rows) {
        case Config::Rows::all:
            for (int i = 0; i < n && i < kRows; i++)
                add(i);
            break;
        case Config::Rows::top10:
            for (int i = 0; i < n && i < 10; i++)
                add(i);
            add(me);
            break;
        case Config::Rows::compact: {
            for (int i = 0; i < 3; i++)
                add(i);
            if (me < 0) {
                for (int i = 3; i < 6; i++)
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

    // ---- one row
    void fill_row(Row &r, const Snapshot &snap, int idx, Kind kind, const Config &cfg, bool lead, double now)
    {
        const CarIn &c = snap.cars[size_t(idx)];
        const CarIn &leader = snap.cars.front();
        const bool race = kind == Kind::race;
        r.on = true;
        r.vid = int(c.vid);
        int pos = c.pos > 0 ? c.pos : idx + 1;
        r.pos = std::to_string(pos);
        r.p1 = pos == 1;
        r.me = cfg.highlight && c.me;
        r.badge = c.badge.empty() ? "" : "#overrides.osd.timing_tower.team_badges_hud." + c.badge;
        r.team = team_index(c.badge);
        r.name = c.name.empty() ? "" : (cfg.abbr && !c.code.empty() ? c.code : c.name);

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
        if (cfg.tyres && !r.out) {
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
            r.fl = snap.s.fastest_vid >= 0 && uint32_t(snap.s.fastest_vid) == c.vid;
            if (retired) {
                r.gap = c.status == 5 ? "DSQ" : c.status == 6 ? "NC" : c.status == 4 ? "DNF" : "OUT";
            } else if (snap.s.red) {
                r.gap = ""; // red flag: the field waits in the pit lane, no live gaps
            } else if (c.in_pit && c.status != 3) {
                r.gap = "PIT";
            } else if (idx == 0) {
                r.gap = lead ? "LEADER" : "INTERVAL";
                r.gap_label = true;
            } else {
                int down = laps_down(snap, idx, lead ? 0 : idx - 1);
                int gap = lead ? c.gap_leader_ms : c.gap_ahead_ms;
                if (down > 0 && (lead || gap < 0))
                    r.gap = fmt_laps_down(down);
                else if (gap >= 0)
                    r.gap = fmt_gap_race(gap);
                else if (lead && c.gap_ahead_ms >= 0 && leader.status != 3)
                    r.gap = "";
            }
            // a new fastest lap flashes the lap time in purple
            if (fastest_flash_vid_ == int(c.vid) && now < fastest_flash_until_ && fastest_ms_ > 0) {
                r.gap = fmt_lap(fastest_ms_);
                r.gap_purple = true;
                r.gap_label = false;
            }
        } else {
            r.pit = c.in_pit;
            int best = c.best_ms;
            int lead_best = leader.best_ms;
            if (best > 0) {
                if (idx == 0 || lead_best <= 0)
                    r.gap = fmt_lap(best);
                else
                    r.gap = fmt_gap_ms(best - lead_best);
            }
            if (c.vid < flash_.size() && now < flash_[c.vid].until && flash_[c.vid].ms > 0) {
                r.gap = fmt_lap(flash_[c.vid].ms);
                r.gap_purple = flash_[c.vid].purple;
                r.gap_green = !flash_[c.vid].purple;
            }
            // knockout zone
            int seg = knockout_segment(snap.s.type);
            if (seg == 1 || seg == 2) {
                int cut = knockout_cut(seg, field_);
                int prev_cut = seg == 2 ? knockout_cut(1, field_) : field_;
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
            trans_card_ = card_vsc_end;
            trans_until_ = now + 4.0;
            clear_at_ = now + 4.0;
        } else if (last_sc_ == 1 && sc == 0 && !s.red) {
            trans_card_ = card_clear;
            trans_until_ = now + 5.0;
            clear_at_ = -1;
        }
        if (sc != 0) {
            trans_card_ = card_none;
            clear_at_ = -1;
        }
        last_sc_ = sc;
        if (trans_card_ == card_vsc_end && now >= trans_until_ && clear_at_ >= 0) {
            trans_card_ = card_clear;
            trans_until_ = now + 4.0;
            clear_at_ = -1;
        }

        if (race && s.chequered)
            return card_chq;
        if (s.red)
            return card_red;
        if (sc == 1)
            return s.sc_in ? card_sc_end : card_sc;
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
        std::string t;
        for (int i = 0; i < 8; i++)
            if (bits & (1u << i)) {
                if (!t.empty())
                    t += " ";
                t += "S" + std::to_string(i + 1);
            }
        return t;
    }

    // ---- interval / leader
    bool lead_mode(const Config &cfg, const Snapshot &snap, double now)
    {
        if (cfg.gap == Config::Gap::leader)
            return true;
        if (cfg.gap == Config::Gap::interval || kind_of(snap.s.type) != Kind::race)
            return false;
        int ll = snap.cars.front().lap;
        if (last_leader_lap_ >= 0 && ll != last_leader_lap_ && ll > 0 && (ll % 5) == 0)
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
            if (fastest_ms_ > 0 || snap.s.demo) {
                fastest_flash_vid_ = snap.s.fastest_vid;
                fastest_flash_until_ = now + 5.0;
            }
            fastest_ms_ = snap.s.fastest_ms;
        }
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
};

} // namespace tvt
